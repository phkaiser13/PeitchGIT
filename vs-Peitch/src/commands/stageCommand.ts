import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerStageCommand(gitAdapter: IGitAdapter) {
    return vscode.commands.registerCommand('peitch.stage', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        try {
            const status = await gitAdapter.getStatus(repoPath);
            const filesToStage = [...status.unstaged, ...status.untracked];

            if (filesToStage.length === 0) {
                vscode.window.showInformationMessage('No changes to stage.');
                return;
            }

            const selectedFiles = await vscode.window.showQuickPick(filesToStage, {
                canPickMany: true,
                placeHolder: 'Select files to stage'
            });

            if (selectedFiles && selectedFiles.length > 0) {
                await gitAdapter.stageFiles(repoPath, selectedFiles);
                vscode.window.showInformationMessage(`Staged ${selectedFiles.length} file(s).`);
                Logger.log(`Staged files: ${selectedFiles.join(', ')}`);
            }
        } catch (error) {
            Logger.showError('Failed to stage files', error);
        }
    });
}
