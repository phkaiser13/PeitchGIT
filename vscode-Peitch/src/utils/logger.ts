import * as vscode from 'vscode';

export class Logger {
    private static outputChannel: vscode.OutputChannel;
    private static context: vscode.ExtensionContext;

    public static init(context: vscode.ExtensionContext): void {
        this.outputChannel = vscode.window.createOutputChannel('Peitch');
        this.context = context;
    }

    public static log(message: string): void {
        if (!this.outputChannel) {
            console.warn("Logger not initialized. Call Logger.init() first.");
            return;
        }
        const timestamp = new Date().toISOString();
        const logMessage = `[${timestamp}] ${message}`;
        this.outputChannel.appendLine(logMessage);

        // Store in global state for audit trail
        const auditTrail = this.context.globalState.get<string[]>('auditTrail', []);
        auditTrail.push(logMessage);
        this.context.globalState.update('auditTrail', auditTrail);
    }

    public static showError(message: string, error?: any): void {
        const fullMessage = error instanceof Error ? `${message}: ${error.message}` : message;
        this.log(`ERROR: ${fullMessage}`);
        if (error?.stack) {
            this.log(error.stack);
        }
        vscode.window.showErrorMessage(fullMessage);
        this.outputChannel.show();
    }
}
