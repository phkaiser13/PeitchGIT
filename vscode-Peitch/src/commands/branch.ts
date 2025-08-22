import * as vscode from 'vscode';
import { phgit } from '../core/phgit';

export function registerBranchCommands(context: vscode.ExtensionContext) {
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.createBranch', async () => {
            const branchName = await vscode.window.showInputBox({
                prompt: 'Enter the new branch name'
            });

            if (branchName) {
                try {
                    await phgit.execute(`checkout -b ${branchName}`);
                    vscode.window.showInformationMessage(`Branch '${branchName}' created and checked out.`);
                    vscode.commands.executeCommand('peitchgit.branches.refresh');
                } catch (error) {
                    vscode.window.showErrorMessage('Failed to create branch.');
                }
            }
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.switchBranch', async () => {
            try {
                const branches = (await phgit.getBranches()).map(b => b.replace('* ', ''));
                const selectedBranch = await vscode.window.showQuickPick(branches, {
                    placeHolder: 'Select a branch to switch to'
                });

                if (selectedBranch) {
                    await phgit.execute(`checkout ${selectedBranch}`);
                    vscode.window.showInformationMessage(`Switched to branch '${selectedBranch}'.`);
                    vscode.commands.executeCommand('peitchgit.branches.refresh');
                }
            } catch (error) {
                vscode.window.showErrorMessage('Failed to switch branch.');
            }
        })
    );

    // Placeholder for merge command
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.mergeBranch', async () => {
            // Implementation for merging a branch
            vscode.window.showInformationMessage('Merge command not yet implemented.');
        })
    );
}
