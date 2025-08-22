import { exec } from 'child_process';
import { promisify } from 'util';
import * as vscode from 'vscode';

const execAsync = promisify(exec);

/**
 * Provides a wrapper for executing phgit commands.
 */
export class Phgit {
    private workspaceRoot: string;

    constructor() {
        if (!vscode.workspace.workspaceFolders || vscode.workspace.workspaceFolders.length === 0) {
            throw new Error("No workspace folder open.");
        }
        this.workspaceRoot = vscode.workspace.workspaceFolders[0].uri.fsPath;
    }

    /**
     * Executes a given phgit command and returns its standard output.
     * @param command The phgit command to execute (e.g., "status", "log").
     * @returns A promise that resolves with the stdout of the command.
     */
    public async execute(command: string): Promise<string> {
        try {
            const { stdout } = await execAsync(`phgit ${command}`, { cwd: this.workspaceRoot });
            return stdout.trim();
        } catch (error: any) {
            // Log the full error to the output channel for debugging
            console.error(`Error executing "phgit ${command}":`, error);
            // Show a more user-friendly error message
            vscode.window.showErrorMessage(`PeitchGit command failed: ${error.stderr || error.message}`);
            throw error;
        }
    }

    // Example high-level functions
    public async getStatus(): Promise<string[]> {
        const output = await this.execute('status --porcelain');
        return output.split('\n').filter(line => line.length > 0);
    }

    public async getBranches(): Promise<string[]> {
        const output = await this.execute('branch');
        return output.split('\n').map(line => line.trim());
    }

    public async getLog(): Promise<string[]> {
        // A more complex log format might be needed for the history view
        const output = await this.execute('log --oneline --graph --decorate');
        return output.split('\n');
    }
}

// Singleton instance to be used across the extension
export const phgit = new Phgit();
