import * as vscode from 'vscode';
import { ChangesProvider } from '../providers/ChangesProvider';

/**
 * Registers and manages the file changes view.
 *
 * This function should be called from the main `activate` function
 * to initialize the UI component that displays file status.
 *
 * @param context The extension context provided by VS Code.
 */
export function registerChangesView(context: vscode.ExtensionContext) {
    const changesProvider = new ChangesProvider();
    vscode.window.createTreeView('peitchgit-changes', {
        treeDataProvider: changesProvider
    });

    // Register a command to refresh the changes view
    context.subscriptions.push(
        vscode.commands.registerCommand('peitchgit.changes.refresh', () => {
            changesProvider.refresh();
        })
    );
}
