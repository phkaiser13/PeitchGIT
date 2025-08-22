import { phgit } from './phgit';
import { EventEmitter } from 'vscode';

/**
 * Represents the state of a single Git repository.
 * It caches data and emits events when the state changes.
 */
export class Repository {
    private _onDidChange = new EventEmitter<void>();
    public readonly onDidChange = this._onDidChange.event;

    private currentBranch: string | null = null;
    private changedFiles: string[] = [];

    constructor() {
        // TODO: Implement file system watcher to automatically refresh state
        this.refresh();
    }

    /**
     * Fetches the latest state from the phgit CLI and updates the repository object.
     */
    public async refresh(): Promise<void> {
        try {
            const branches = await phgit.getBranches();
            this.currentBranch = branches.find(b => b.startsWith('*'))?.substring(2) || null;

            this.changedFiles = await phgit.getStatus();

            this._onDidChange.fire(); // Notify listeners of the change
        } catch (error) {
            console.error("Failed to refresh repository state:", error);
        }
    }

    public getCurrentBranch(): string | null {
        return this.currentBranch;
    }

    public getChangedFiles(): string[] {
        return this.changedFiles;
    }
}
