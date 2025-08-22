import * as vscode from 'vscode';
import { phgit } from '../core/phgit';
import { GraphViewPanel } from '../views/graphView';

export function registerAdvancedCommands(context: vscode.ExtensionContext) {
    // Placeholder for rebase command
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.rebase', async () => {
            vscode.window.showInformationMessage('Rebase command not yet implemented.');
        })
    );

    // Placeholder for stash command
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.stash', async () => {
            try {
                await phgit.execute('stash');
                vscode.window.showInformationMessage('Changes stashed.');
                vscode.commands.executeCommand('peitchgit.changes.refresh');
            } catch (error) {
                vscode.window.showErrorMessage('Stash failed.');
            }
        })
    );

    // Command to show the graph view
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.showGraph', () => {
            GraphViewPanel.createOrShow(vscode.Uri.file(context.extensionPath));
        })
    );
}