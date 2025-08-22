import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';

function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function registerGenerateCommitWithAICommand(gitAdapter: IGitAdapter) {
    return vscode.commands.registerCommand('peitch.generateCommitWithAI', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('No repository found.');
            return;
        }

        try {
            const stagedFiles = await gitAdapter.getStagedFiles(repoPath);
            if (stagedFiles.length === 0) {
                vscode.window.showInformationMessage('No files staged. Stage files to generate a commit message.');
                return;
            }

            // Mock AI logic
            const fileSummary = stagedFiles.join(', ');
            const messages = [
                {
                    label: "feat: Implement new feature",
                    detail: `Adds new capabilities based on changes in ${fileSummary}`
                },
                {
                    label: "fix: Correct a bug",
                    detail: `Resolves an issue related to ${fileSummary}`
                },
                {
                    label: "refactor: Improve code structure",
                    detail: `Enhances code readability and maintainability in ${fileSummary}`
                }
            ];

            const selected = await vscode.window.showQuickPick(messages, {
                placeHolder: 'Select a generated commit message (mock)'
            });

            if (selected) {
                const commitSha = await gitAdapter.commit(repoPath, selected.label);
                const shortSha = commitSha.substring(0, 7);
                vscode.window.showInformationMessage(`Commit successful with AI message: ${shortSha}`);
                Logger.log(`Committed with AI message "${selected.label}" (sha: ${commitSha})`);
            }

        } catch (error) {
            Logger.showError('Failed to generate AI commit message or commit', error);
        }
    });
}
