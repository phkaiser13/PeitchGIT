import * as vscode from 'vscode';
import * as path from 'path';
import { IGitAdapter } from '../git/types';
import { WebviewToExtensionMessage } from '../common/types';
import { Logger } from '../utils/logger';

export class WebviewController {
    public static currentPanel: WebviewController | undefined;
    private readonly panel: vscode.WebviewPanel;
    private readonly extensionUri: vscode.Uri;
    private readonly gitAdapter: IGitAdapter;
    private disposables: vscode.Disposable[] = [];

    private constructor(panel: vscode.WebviewPanel, extensionUri: vscode.Uri, gitAdapter: IGitAdapter) {
        this.panel = panel;
        this.extensionUri = extensionUri;
        this.gitAdapter = gitAdapter;

        this.update();

        this.panel.onDidDispose(() => this.dispose(), null, this.disposables);

        this.panel.webview.onDidReceiveMessage(
            (message: WebviewToExtensionMessage) => this.handleMessage(message),
            null,
            this.disposables
        );
    }

    public static createOrShow(extensionUri: vscode.Uri, gitAdapter: IGitAdapter) {
        const column = vscode.window.activeTextEditor?.viewColumn || vscode.ViewColumn.One;

        if (WebviewController.currentPanel) {
            WebviewController.currentPanel.panel.reveal(column);
            return;
        }

        const panel = vscode.window.createWebviewPanel(
            'peitchPanel',
            'Peitch',
            column,
            {
                enableScripts: true,
                localResourceRoots: [vscode.Uri.file(path.join(extensionUri.fsPath, 'dist'))]
            }
        );

        WebviewController.currentPanel = new WebviewController(panel, extensionUri, gitAdapter);
    }

    public dispose() {
        WebviewController.currentPanel = undefined;
        this.panel.dispose();
        while (this.disposables.length) {
            const x = this.disposables.pop();
            if (x) {
                x.dispose();
            }
        }
    }

    public showBranch(branchName: string) {
        this.panel.webview.postMessage({ type: 'showBranch', payload: { branch: branchName } });
    }
    
    private getWorkspacePath(): string | undefined {
        return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
    }

    private async handleMessage(message: WebviewToExtensionMessage) {
        const repoPath = this.getWorkspacePath();
        if (!repoPath) {
            this.panel.webview.postMessage({ type: 'error', payload: { message: 'No workspace open.' }});
            return;
        }

        try {
            switch (message.type) {
                case 'ready':
                case 'getStatus': {
                    const status = await this.gitAdapter.getStatus(repoPath);
                    this.panel.webview.postMessage({ type: 'status', payload: status });
                    break;
                }
                case 'getCommitLog': {
                    const log = await this.gitAdapter.getCommitLog(repoPath, 50);
                    this.panel.webview.postMessage({ type: 'commitLog', payload: log });
                    break;
                }
                case 'getFilesForBranch': {
                    const files = await this.gitAdapter.getFilesForBranch(repoPath, message.payload.branch);
                    this.panel.webview.postMessage({ type: 'branchFiles', payload: { branch: message.payload.branch, files } });
                    break;
                }
                case 'stageFiles': {
                    await this.gitAdapter.stageFiles(repoPath, message.payload.files);
                    const status = await this.gitAdapter.getStatus(repoPath);
                    this.panel.webview.postMessage({ type: 'status', payload: status });
                    break;
                }
                case 'unstageFiles': {
                    await this.gitAdapter.unstageFiles(repoPath, message.payload.files);
                    const status = await this.gitAdapter.getStatus(repoPath);
                    this.panel.webview.postMessage({ type: 'status', payload: status });
                    break;
                }
                case 'commit': {
                    await this.gitAdapter.commit(repoPath, message.payload.message);
                    const status = await this.gitAdapter.getStatus(repoPath);
                    this.panel.webview.postMessage({ type: 'status', payload: status });
                    this.panel.webview.postMessage({ type: 'operationSuccess', payload: { message: 'Commit successful!' } });
                    break;
                }
                case 'fetch': {
                    await this.gitAdapter.fetch(repoPath);
                    this.panel.webview.postMessage({ type: 'operationSuccess', payload: { message: 'Fetch successful!' } });
                    const status = await this.gitAdapter.getStatus(repoPath);
                    this.panel.webview.postMessage({ type: 'status', payload: status });
                    break;
                }
                 case 'push': {
                    await this.gitAdapter.push(repoPath);
                    this.panel.webview.postMessage({ type: 'operationSuccess', payload: { message: 'Push successful!' } });
                    const status = await this.gitAdapter.getStatus(repoPath);
                    this.panel.webview.postMessage({ type: 'status', payload: status });
                    break;
                }
                case 'showError': {
                    Logger.showError(message.payload.message);
                    break;
                }
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : 'An unknown error occurred.';
            Logger.showError('Failed to execute git command', error);
            this.panel.webview.postMessage({ type: 'error', payload: { message: errorMessage } });
        }
    }

    private update() {
        this.panel.webview.html = this.getHtmlForWebview();
    }

    private getHtmlForWebview(): string {
        const scriptPath = vscode.Uri.file(path.join(this.extensionUri.fsPath, 'dist', 'webview.js'));
        const scriptUri = scriptPath.with({ scheme: 'vscode-resource' });
        const nonce = getNonce();

        return `<!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline' vscode-resource:; script-src 'nonce-${nonce}';">
                <title>Peitch</title>
            </head>
            <body>
                <div id="root"></div>
                <script nonce="${nonce}" src="${scriptUri}"></script>
            </body>
            </html>`;
    }
}

function getNonce() {
    let text = '';
    const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    for (let i = 0; i < 32; i++) {
        text += possible.charAt(Math.floor(Math.random() * possible.length));
    }
    return text;
}