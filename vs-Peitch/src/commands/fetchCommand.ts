import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerFetchCommand(gitAdapter: IGitAdapter) {
    return vscode.commands.registerCommand('peitch.fetch', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        try {
            await gitAdapter.fetch(repoPath);
            vscode.window.showInformationMessage('Fetch successful!');
            Logger.log('Fetch command executed successfully.');
        } catch (error) {
            Logger.showError('Fetch failed', error);
        }
    });
}
