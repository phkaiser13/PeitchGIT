import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerStatusCommand(gitAdapter: IGitAdapter) {
    return vscode.commands.registerCommand('peitch.status', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found. Open a folder with a Git repository.');
            return;
        }

        try {
            const status = await gitAdapter.getStatus(repoPath);
            const statusMessage = `Branch: ${status.branch} | Staged: ${status.staged.length} | Unstaged: ${status.unstaged.length} | Untracked: ${status.untracked.length}`;
            vscode.window.showInformationMessage(statusMessage);
            Logger.log('Status command executed successfully.');
            Logger.log(JSON.stringify(status, null, 2));
        } catch (error) {
            Logger.showError('Failed to get Git status', error);
        }
    });
}
