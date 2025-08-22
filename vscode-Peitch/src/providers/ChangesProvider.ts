import * as vscode from 'vscode';
import { phgit } from '../core/phgit';

/**
 * Provides the data for the file changes tree view.
 */
export class ChangesProvider implements vscode.TreeDataProvider<ChangeItem> {
    private _onDidChangeTreeData: vscode.EventEmitter<ChangeItem | undefined | null> = new vscode.EventEmitter<ChangeItem | undefined | null>();
    readonly onDidChangeTreeData: vscode.Event<ChangeItem | undefined | null> = this._onDidChangeTreeData.event;

    refresh(): void {
        this._onDidChangeTreeData.fire(undefined);
    }

    getTreeItem(element: ChangeItem): vscode.TreeItem {
        return element;
    }

    async getChildren(element?: ChangeItem): Promise<ChangeItem[]> {
        if (element) {
            // Files are top-level, no children
            return [];
        }

        try {
            const statusLines = await phgit.getStatus();
            return statusLines.map(line => {
                const status = line.substring(0, 2).trim();
                const filePath = line.substring(3);
                return new ChangeItem(filePath, status, vscode.TreeItemCollapsibleState.None);
            });
        } catch (error) {
            vscode.window.showErrorMessage('Failed to fetch Git status.');
            return [];
        }
    }
}

/**
 * Represents a single changed file in the tree view.
 */
class ChangeItem extends vscode.TreeItem {
    constructor(
        public readonly filePath: string,
        public readonly status: string,
        public readonly collapsibleState: vscode.TreeItemCollapsibleState
    ) {
        super(filePath, collapsibleState);
        this.label = filePath;
        this.description = this.getStatusDescription(status);
        this.tooltip = `${this.filePath} - ${this.description}`;
        // You can set commands to be executed on click
        // this.command = { ... };
    }

    private getStatusDescription(status: string): string {
        switch (status) {
            case 'M': return 'Modified';
            case 'A': return 'Added';
            case 'D': return 'Deleted';
            case 'R': return 'Renamed';
            case 'C': return 'Copied';
            case 'U': return 'Unmerged';
            case '??': return 'Untracked';
            default: return 'Changed';
        }
    }

    // iconPath could change based on file status
}