import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';
import * as path from 'path';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerWorktreeCommands(gitAdapter: IGitAdapter): vscode.Disposable[] {
    const subscriptions: vscode.Disposable[] = [];

    subscriptions.push(vscode.commands.registerCommand('peitch.manageWorktrees', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        const action = await vscode.window.showQuickPick(['Add Worktree', 'Remove Worktree'], {
            placeHolder: 'What would you like to do?'
        });

        if (!action) return;

        if (action === 'Add Worktree') {
            await addWorktree(gitAdapter, repoPath);
        } else if (action === 'Remove Worktree') {
            await removeWorktree(gitAdapter, repoPath);
        }
        
        vscode.commands.executeCommand('peitch.refreshTreeView');
    }));

    return subscriptions;
}

async function addWorktree(gitAdapter: IGitAdapter, repoPath: string) {
    try {
        const branches = await gitAdapter.getBranches(repoPath);
        const selectedBranch = await vscode.window.showQuickPick(branches, {
            placeHolder: 'Select a branch to create a worktree for'
        });
        if (!selectedBranch) return;
        
        const worktreePath = await vscode.window.showInputBox({
            prompt: 'Enter the path for the new worktree (relative to repository root)',
            value: `../${path.basename(repoPath)}-${selectedBranch.replace(/[/\\?%*:|"<>]/g, '-')}`
        });
        if (!worktreePath) return;

        await gitAdapter.addWorktree(repoPath, worktreePath, selectedBranch);
        vscode.window.showInformationMessage(`Worktree for branch '${selectedBranch}' created at '${worktreePath}'.`);
        Logger.log(`Added worktree for ${selectedBranch} at ${worktreePath}`);

    } catch (error) {
        Logger.showError('Failed to add worktree', error);
    }
}

async function removeWorktree(gitAdapter: IGitAdapter, repoPath: string) {
    try {
        const worktrees = await gitAdapter.getWorktrees(repoPath);
        const removableWorktrees = worktrees.filter(w => !w.isCurrent && w.path !== repoPath);

        if (removableWorktrees.length === 0) {
            vscode.window.showInformationMessage('No additional worktrees to remove.');
            return;
        }

        const items = removableWorktrees.map(w => ({
            label: path.basename(w.path),
            description: `(branch: ${w.branch})`,
            path: w.path
        }));
        
        const selectedWorktree = await vscode.window.showQuickPick(items, {
            placeHolder: 'Select a worktree to remove'
        });
        if (!selectedWorktree) return;

        await gitAdapter.removeWorktree(repoPath, selectedWorktree.path);
        vscode.window.showInformationMessage(`Worktree at '${selectedWorktree.path}' removed.`);
        Logger.log(`Removed worktree at ${selectedWorktree.path}`);
    } catch (error) {
        Logger.showError('Failed to remove worktree', error);
    }
}
