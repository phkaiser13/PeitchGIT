import * as vscode from 'vscode';
import { phgit } from '../core/phgit';

/**
 * Provides the data for the branches tree view.
 */
export class BranchesProvider implements vscode.TreeDataProvider<BranchItem> {
    private _onDidChangeTreeData: vscode.EventEmitter<BranchItem | undefined | null> = new vscode.EventEmitter<BranchItem | undefined | null>();
    readonly onDidChangeTreeData: vscode.Event<BranchItem | undefined | null> = this._onDidChangeTreeData.event;

    refresh(): void {
        this._onDidChangeTreeData.fire(undefined);
    }

    getTreeItem(element: BranchItem): vscode.TreeItem {
        return element;
    }

    async getChildren(element?: BranchItem): Promise<BranchItem[]> {
        if (element) {
            // Branches are top-level, no children
            return [];
        }
        
        try {
            const branchLines = await phgit.getBranches();
            return branchLines.map(line => {
                const isCurrent = line.startsWith('*');
                const name = isCurrent ? line.substring(2) : line;
                return new BranchItem(name, isCurrent, vscode.TreeItemCollapsibleState.None);
            });
        } catch (error) {
            vscode.window.showErrorMessage('Failed to fetch Git branches.');
            return [];
        }
    }
}

/**
 * Represents a single branch in the tree view.
 */
class BranchItem extends vscode.TreeItem {
    constructor(
        public readonly branchName: string,
        public readonly isCurrent: boolean,
        public readonly collapsibleState: vscode.TreeItemCollapsibleState
    ) {
        super(branchName, collapsibleState);
        this.label = branchName;
        this.description = isCurrent ? 'current' : '';
        this.tooltip = `Branch: ${this.branchName}`;
        this.contextValue = isCurrent ? 'currentBranch' : 'branch';
    }

    iconPath = { id: 'git-branch' } as any;
}