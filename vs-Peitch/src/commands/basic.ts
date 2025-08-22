import * as vscode from 'vscode';
import { phgit } from '../core/phgit';

export function registerBasicCommands(context: vscode.ExtensionContext) {
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.commit', async () => {
            const message = await vscode.window.showInputBox({
                prompt: 'Enter commit message'
            });

            if (message) {
                try {
                    await phgit.execute(`commit -m "${message}"`);
                    vscode.window.showInformationMessage('Commit successful!');
                    // Refresh relevant views
                    vscode.commands.executeCommand('peitchgit.changes.refresh');
                    vscode.commands.executeCommand('peitchgit.history.refresh');
                } catch (error) {
                    vscode.window.showErrorMessage('Commit failed.');
                }
            }
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.push', async () => {
            try {
                await phgit.execute('push');
                vscode.window.showInformationMessage('Push successful!');
            } catch (error) {
                vscode.window.showErrorMessage('Push failed.');
            }
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.pull', async () => {
            try {
                await phgit.execute('pull');
                vscode.window.showInformationMessage('Pull successful!');
                 // Refresh relevant views
                 vscode.commands.executeCommand('peitchgit.changes.refresh');
                 vscode.commands.executeCommand('peitchgit.history.refresh');
            } catch (error) {
                vscode.window.showErrorMessage('Pull failed.');
            }
        })
    );
}
