import * as vscode from 'vscode';
import { phgit } from '../core/phgit';

/**
 * Provides the data for the commit history tree view.
 */
export class HistoryProvider implements vscode.TreeDataProvider<HistoryItem> {
    private _onDidChangeTreeData: vscode.EventEmitter<HistoryItem | undefined | null> = new vscode.EventEmitter<HistoryItem | undefined | null>();
    readonly onDidChangeTreeData: vscode.Event<HistoryItem | undefined | null> = this._onDidChangeTreeData.event;

    refresh(): void {
        this._onDidChangeTreeData.fire();
    }

    getTreeItem(element: HistoryItem): vscode.TreeItem {
        return element;
    }

    async getChildren(element?: HistoryItem): Promise<HistoryItem[]> {
        if (element) {
            // Commits are top-level, no children for now
            return [];
        }

        try {
            const logEntries = await phgit.getLog();
            return logEntries.map(entry => new HistoryItem(entry, vscode.TreeItemCollapsibleState.None));
        } catch (error) {
            vscode.window.showErrorMessage('Failed to fetch Git history.');
            return [];
        }
    }
}

/**
 * Represents a single commit in the history tree view.
 */
class HistoryItem extends vscode.TreeItem {
    constructor(
        public readonly label: string,
        public readonly collapsibleState: vscode.TreeItemCollapsibleState
    ) {
        super(label, collapsibleState);
        this.tooltip = `Commit: ${this.label}`;
        this.description = ''; // Could be author, date, etc.
    }

    // iconPath can be used to show a commit icon
    // contextValue = 'commit';
}