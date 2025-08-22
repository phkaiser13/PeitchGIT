import * as vscode from 'vscode';

let outputChannel: vscode.OutputChannel;

/**
 * Gets a singleton output channel for the extension.
 * @returns The VS Code OutputChannel for PeitchGit.
 */
function getOutputChannel(): vscode.OutputChannel {
    if (!outputChannel) {
        outputChannel = vscode.window.createOutputChannel('PeitchGit');
    }
    return outputChannel;
}

/**
 * Handles errors by logging them to the output channel and showing a notification.
 * @param error The error object or string.
 * @param message A user-friendly message to display.
 */
export function handleError(error: any, message: string) {
    const channel = getOutputChannel();
    channel.appendLine(`[ERROR] ${new Date().toISOString()}: ${message}`);

    if (error instanceof Error) {
        channel.appendLine(error.stack || error.message);
    } else {
        channel.appendLine(String(error));
    }
    
    channel.show(true); // Preserve focus on the editor
    vscode.window.showErrorMessage(`${message}. See the PeitchGit output for more details.`);
}
