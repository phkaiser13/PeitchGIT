import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerCommitCommand(gitAdapter: IGitAdapter) {
    return vscode.commands.registerCommand('peitch.commit', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }
        
        const stagedFiles = await gitAdapter.getStagedFiles(repoPath);
        if (stagedFiles.length === 0) {
            Logger.showError('No files staged for commit.');
            return;
        }

        const message = await vscode.window.showInputBox({ prompt: 'Enter commit message' });
        if (!message) {
            Logger.log('Commit cancelled by user.');
            return;
        }

        try {
            const commitSha = await gitAdapter.commit(repoPath, message);
            const shortSha = commitSha.substring(0, 7);
            vscode.window.showInformationMessage(`Commit successful: ${shortSha}`);
            Logger.log(`Committed with message "${message}" (sha: ${commitSha})`);
        } catch (error) {
            Logger.showError('Commit failed', error);
        }
    });
}
