import * as vscode from 'vscode';
import { HistoryProvider } from '../providers/HistoryProvider';

/**
 * Registers and manages the commit history view.
 *
 * This function is intended to be called from the main `activate` function
 * to set up the UI components related to commit history.
 *
 * @param context The extension context provided by VS Code.
 */
export function registerHistoryView(context: vscode.ExtensionContext) {
    const historyProvider = new HistoryProvider();
    vscode.window.createTreeView('peitchgit-history', {
        treeDataProvider: historyProvider
    });

    // You can add commands to refresh the view
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.history.refresh', () => {
            historyProvider.refresh();
        })
    );
}
