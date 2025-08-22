import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';
import { getSettings } from '../utils/settings';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerRemoteCommands(gitAdapter: IGitAdapter): vscode.Disposable[] {
    const subscriptions: vscode.Disposable[] = [];

    subscriptions.push(vscode.commands.registerCommand('peitch.addRemote', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        try {
            const remoteName = await vscode.window.showInputBox({ prompt: 'Enter remote name' });
            if (!remoteName) return;

            const remoteUrl = await vscode.window.showInputBox({ prompt: 'Enter remote URL' });
            if (!remoteUrl) return;
            
            await gitAdapter.addRemote(repoPath, remoteName, remoteUrl);
            vscode.window.showInformationMessage(`Remote '${remoteName}' added successfully.`);
            Logger.log(`Added remote ${remoteName} with URL ${remoteUrl}`);
            vscode.commands.executeCommand('peitch.refreshTreeView');
        } catch (error) {
            Logger.showError('Failed to add remote', error);
        }
    }));

    subscriptions.push(vscode.commands.registerCommand('peitch.pushToAll', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        const settings = getSettings();
        const mirrors = settings.mirrors;
        if (!mirrors || mirrors.length === 0) {
            vscode.window.showInformationMessage("No mirror remotes configured in 'peitch.mirrors' setting.");
            return;
        }

        const status = await gitAdapter.getStatus(repoPath);

        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: `Pushing to all remotes...`,
            cancellable: false
        }, async (progress) => {
            for (const remote of mirrors) {
                progress.report({ message: `Pushing to ${remote}...` });
                try {
                    await gitAdapter.pushToRemote(repoPath, remote, status.branch);
                    Logger.log(`Successfully pushed to ${remote}`);
                } catch (error) {
                    Logger.showError(`Failed to push to remote '${remote}'`, error);
                }
            }
        });
        vscode.window.showInformationMessage('Finished pushing to all configured remotes.');
    }));

    return subscriptions;
}
