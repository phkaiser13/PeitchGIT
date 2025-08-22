import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerCheckoutCommand(gitAdapter: IGitAdapter) {
    return vscode.commands.registerCommand('peitch.checkout', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        try {
            const branches = await gitAdapter.getBranches(repoPath);
            const createNewBranchOption = '$(plus) Create New Branch...';
            const selected = await vscode.window.showQuickPick([createNewBranchOption, ...branches], {
                placeHolder: 'Select a branch to switch to or create a new one'
            });

            if (!selected) {
                return; // User cancelled
            }

            if (selected === createNewBranchOption) {
                const newBranchName = await vscode.window.showInputBox({
                    prompt: 'Enter the new branch name'
                });
                if (newBranchName) {
                    await gitAdapter.checkout(repoPath, newBranchName, true);
                    vscode.window.showInformationMessage(`Created and switched to new branch '${newBranchName}'.`);
                    Logger.log(`Created and checked out branch: ${newBranchName}`);
                }
            } else {
                await gitAdapter.checkout(repoPath, selected, false);
                vscode.window.showInformationMessage(`Switched to branch '${selected}'.`);
                Logger.log(`Checked out branch: ${selected}`);
            }
        } catch (error) {
            Logger.showError('Failed to switch branch', error);
        }
    });
}
