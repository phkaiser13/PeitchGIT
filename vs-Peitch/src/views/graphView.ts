import * as vscode from 'vscode';
import * as path from 'path';

/**
 * Manages the webview panel for displaying the commit graph.
 */
export class GraphViewPanel {
    public static currentPanel: GraphViewPanel | undefined;
    private readonly panel: vscode.WebviewPanel;
    private readonly extensionUri: vscode.Uri;
    private disposables: vscode.Disposable[] = [];

    private constructor(panel: vscode.WebviewPanel, extensionUri: vscode.Uri) {
        this.panel = panel;
        this.extensionUri = extensionUri;

        this.update();

        this.panel.onDidDispose(() => this.dispose(), null, this.disposables);

        this.panel.webview.onDidReceiveMessage(
            message => {
                switch (message.command) {
                    case 'alert':
                        vscode.window.showErrorMessage(message.text);
                        return;
                }
            },
            null,
            this.disposables
        );
    }

    public static createOrShow(extensionUri: vscode.Uri) {
        const column = vscode.window.activeTextEditor
            ? vscode.window.activeTextEditor.viewColumn
            : undefined;

        if (GraphViewPanel.currentPanel) {
            GraphViewPanel.currentPanel.panel.reveal(column);
            return;
        }

        const panel = vscode.window.createWebviewPanel(
            'peitchgitGraph',
            'PeitchGit Graph',
            column || vscode.ViewColumn.One,
            {
                enableScripts: true,
                localResourceRoots: [vscode.Uri.file(path.join(extensionUri.fsPath, 'dist'))]
            }
        );

        GraphViewPanel.currentPanel = new GraphViewPanel(panel, extensionUri);
    }

    private update() {
        this.panel.webview.html = this.getHtmlForWebview();
    }

    private getHtmlForWebview(): string {
        const scriptPath = vscode.Uri.file(path.join(this.extensionUri.fsPath, 'dist', 'webviews', 'graph', 'main.js'));
        const stylePath = vscode.Uri.file(path.join(this.extensionUri.fsPath, 'webviews', 'graph', 'style.css'));

        const scriptUri = scriptPath.with({ scheme: 'vscode-resource' });
        const styleUri = stylePath.with({ scheme: 'vscode-resource' });

        // Use a nonce to only allow specific scripts to be run
        const nonce = getNonce();

        return `<!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src vscode-resource:; script-src 'nonce-${nonce}';">
                <link href="${styleUri}" rel="stylesheet">
                <title>PeitchGit Graph</title>
            </head>
            <body>
                <h1>Commit Graph</h1>
                <div id="root"></div>
                <script type="module" nonce="${nonce}" src="${scriptUri}"></script>
            </body>
            </html>`;
    }

    public dispose() {
        GraphViewPanel.currentPanel = undefined;
        this.panel.dispose();
        while (this.disposables.length) {
            const x = this.disposables.pop();
            if (x) {
                x.dispose();
            }
        }
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