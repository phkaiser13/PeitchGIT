// This script is executed in the context of the webview
// It can't directly access the VS Code API
// It communicates with the extension using acquireVsCodeApi()

// @ts-ignore
const vscode = acquireVsCodeApi();

window.addEventListener('load', main);

function main() {
    const root = document.getElementById('root');
    if (root) {
        root.innerHTML = `<p>Graph rendering logic goes here.</p>
        <button id="test-button">Send Message to Extension</button>`;

        const button = document.getElementById('test-button');
        button?.addEventListener('click', () => {
            vscode.postMessage({
                command: 'alert',
                text: 'Hello from the webview!'
            });
        });
    }

    // Example of receiving a message from the extension
    window.addEventListener('message', event => {
        const message = event.data; // The JSON data sent from the extension
        switch (message.command) {
            case 'updateGraph':
                // Code to render the graph with new data
                if (root) {
                    root.innerHTML = `<p>Received new graph data!</p>`;
                }
                break;
        }
    });
}
