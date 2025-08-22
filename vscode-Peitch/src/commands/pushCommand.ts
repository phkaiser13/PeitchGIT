import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerPushCommand(gitAdapter: IGitAdapter) {
    return vscode.commands.registerCommand('peitch.push', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        try {
            await gitAdapter.push(repoPath);
            vscode.window.showInformationMessage('Push successful!');
            Logger.log('Push command executed successfully.');
        } catch (error) {
            Logger.showError('Push failed', error);
        }
    });
}
