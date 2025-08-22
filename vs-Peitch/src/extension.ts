import * as vscode from 'vscode';
import { GitAdapter } from './git/gitAdapter';
import { Logger } from './utils/logger';
import { registerStatusCommand } from './commands/statusCommand';
import { registerStageCommand } from './commands/stageCommand';
import { registerCommitCommand } from './commands/commitCommand';
import { registerPushCommand } from './commands/pushCommand';
import { registerFetchCommand } from './commands/fetchCommand';
import { registerCheckoutCommand } from './commands/checkoutCommand';
import { RepoTreeProvider } from './tree/RepoTreeProvider';
import { WebviewController } from './webview/WebviewController';
import { initializeKeychain } from './ia/keychain';
import { registerAICommands } from './commands/aiCommands';
import { registerRemoteCommands } from './commands/remoteCommands';
import { registerDeployCommands } from './commands/deployCommands';
import { registerWorktreeCommands } from './commands/worktreeCommands';

/**
 * This method is called when your extension is activated.
 */
export function activate(context: vscode.ExtensionContext) {
    Logger.init(context);
    initializeKeychain(context);
    Logger.log('Congratulations, your extension "peitch" is now active!');

    const gitAdapter = new GitAdapter();

    // Register Tree View
    const repoTreeProvider = new RepoTreeProvider(gitAdapter);
    vscode.window.createTreeView('peitch.repos', {
        treeDataProvider: repoTreeProvider
    });
    context.subscriptions.push(
        vscode.commands.registerCommand('peitch.refreshTreeView', () => repoTreeProvider.refresh())
    );


    // Register Webview Panel Command
    context.subscriptions.push(
        vscode.commands.registerCommand('peitch.showPanel', () => {
            WebviewController.createOrShow(vscode.Uri.file(context.extensionPath), gitAdapter);
        })
    );
    
    // When a branch is clicked in the tree view, open the panel and tell it which branch
    context.subscriptions.push(
        vscode.commands.registerCommand('peitch.showBranchView', (branchName: string) => {
             WebviewController.createOrShow(vscode.Uri.file(context.extensionPath), gitAdapter);
             if (WebviewController.currentPanel) {
                WebviewController.currentPanel.showBranch(branchName);
             }
        })
    );

    // Register legacy commands
    context.subscriptions.push(registerStatusCommand(gitAdapter));
    context.subscriptions.push(registerStageCommand(gitAdapter));
    context.subscriptions.push(registerCommitCommand(gitAdapter));
    context.subscriptions.push(registerPushCommand(gitAdapter));
    context.subscriptions.push(registerFetchCommand(gitAdapter));
    context.subscriptions.push(registerCheckoutCommand(gitAdapter));
    
    // Register AI commands
    context.subscriptions.push(...registerAICommands(context, gitAdapter));

    // Register advanced commands
    context.subscriptions.push(...registerRemoteCommands(gitAdapter));
    context.subscriptions.push(...registerDeployCommands());
    context.subscriptions.push(...registerWorktreeCommands(gitAdapter));
}

/**
 * This method is called when your extension is deactivated.
 */
export function deactivate() { }
