import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import * as path from 'path';

type TreeItem = RepoItem | BranchItem | CollapsibleCategoryItem | RemoteItem | TagItem | WorktreeItem;

export class RepoTreeProvider implements vscode.TreeDataProvider<TreeItem> {
    private _onDidChangeTreeData: vscode.EventEmitter<TreeItem | undefined | null> = new vscode.EventEmitter();
    readonly onDidChangeTreeData: vscode.Event<TreeItem | undefined | null> = this._onDidChangeTreeData.event;

    constructor(private gitAdapter: IGitAdapter) {
        // Auto-refresh on git changes in the future
    }

    getTreeItem(element: TreeItem): vscode.TreeItem {
        return element;
    }

    async getChildren(element?: TreeItem): Promise<TreeItem[]> {
        const workspacePath = this.getWorkspacePath();
        if (!workspacePath) {
            return [];
        }

        if (element instanceof RepoItem) {
            const status = await this.gitAdapter.getStatus(workspacePath);
            element.description = status.branch;

            return [
                new CollapsibleCategoryItem('Branches', 'branches'),
                new CollapsibleCategoryItem('Remotes', 'remotes'),
                new CollapsibleCategoryItem('Tags', 'tags'),
                new CollapsibleCategoryItem('Worktrees', 'worktrees'),
            ];
        } else if (element instanceof CollapsibleCategoryItem) {
            switch (element.contextValue) {
                case 'branches': {
                    const status = await this.gitAdapter.getStatus(workspacePath);
                    const branches = await this.gitAdapter.getBranches(workspacePath);
                    return branches.map(b => new BranchItem(b, status.branch === b, vscode.TreeItemCollapsibleState.None));
                }
                case 'remotes': {
                    const remotes = await this.gitAdapter.getRemotes(workspacePath);
                    return remotes.map(r => new RemoteItem(r.name, r.url));
                }
                case 'tags': {
                    const tags = await this.gitAdapter.getTags(workspacePath);
                    return tags.map(t => new TagItem(t));
                }
                case 'worktrees': {
                    const worktrees = await this.gitAdapter.getWorktrees(workspacePath);
                    return worktrees.map(w => new WorktreeItem(path.basename(w.path), w.branch, w.isCurrent, w.path));
                }
            }
        } else {
            const repoName = path.basename(workspacePath);
            return [new RepoItem(repoName, vscode.TreeItemCollapsibleState.Expanded)];
        }
        return [];
    }

    refresh(): void {
        this._onDidChangeTreeData.fire();
    }

    private getWorkspacePath(): string | undefined {
        return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
    }
}

class RepoItem extends vscode.TreeItem {
    constructor(
        public readonly label: string,
        public readonly collapsibleState: vscode.TreeItemCollapsibleState
    ) {
        super(label, collapsibleState);
        this.tooltip = `Repository: ${this.label}`;
        this.iconPath = { id: 'repo' } as unknown as vscode.ThemeIcon;
        this.command = {
            command: 'peitch.showPanel',
            title: 'Show Peitch Panel',
        };
    }
}

class BranchItem extends vscode.TreeItem {
    constructor(
        public readonly label: string,
        public readonly isCurrent: boolean,
        public readonly collapsibleState: vscode.TreeItemCollapsibleState
    ) {
        super(label, collapsibleState);
        this.description = isCurrent ? ' (current)' : '';
        this.tooltip = `Branch: ${this.label}`;
        this.iconPath = { id: 'git-branch' } as unknown as vscode.ThemeIcon;
        this.command = {
            command: 'peitch.showBranchView',
            title: 'Show Branch View',
            arguments: [this.label],
        };
        this.contextValue = 'branch';
    }
}

class CollapsibleCategoryItem extends vscode.TreeItem {
    constructor(public readonly label: string, public readonly contextValue: string) {
        super(label, vscode.TreeItemCollapsibleState.Collapsed);
        this.iconPath = this.getIcon(contextValue);
    }
    private getIcon(context: string): vscode.ThemeIcon {
        switch(context) {
            case 'remotes': return { id: 'cloud' } as unknown as vscode.ThemeIcon;
            case 'tags': return { id: 'tag' } as unknown as vscode.ThemeIcon;
            case 'worktrees': return { id: 'folder-opened' } as unknown as vscode.ThemeIcon;
            default: return { id: 'folder' } as unknown as vscode.ThemeIcon;
        }
    }
}

class RemoteItem extends vscode.TreeItem {
    constructor(public readonly name: string, public readonly url: string) {
        super(name, vscode.TreeItemCollapsibleState.None);
        this.description = url;
        this.tooltip = `Remote: ${name}\nURL: ${url}`;
        this.iconPath = { id: 'cloud-upload' } as unknown as vscode.ThemeIcon;
        this.contextValue = 'remote';
    }
}

class TagItem extends vscode.TreeItem {
    constructor(public readonly name: string) {
        super(name, vscode.TreeItemCollapsibleState.None);
        this.tooltip = `Tag: ${name}`;
        this.iconPath = { id: 'tag' } as unknown as vscode.ThemeIcon;
        this.contextValue = 'tag';
    }
}

class WorktreeItem extends vscode.TreeItem {
    constructor(public readonly name: string, public readonly branch: string, isCurrent: boolean, public readonly path: string) {
        super(name, vscode.TreeItemCollapsibleState.None);
        this.description = `${branch}${isCurrent ? ' (current)' : ''}`;
        this.tooltip = `Worktree: ${path}\nBranch: ${branch}`;
        this.iconPath = { id: 'file-submodule' } as unknown as vscode.ThemeIcon;
        this.contextValue = 'worktree';
    }
}