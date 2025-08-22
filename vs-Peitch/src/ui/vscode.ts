import { WebviewToExtensionMessage } from "../common/types";

// This interface is cast to the VS Code API object.
interface VsCodeApi {
    postMessage(message: WebviewToExtensionMessage): void;
    getState(): any;
    setState(newState: any): void;
}

// The acquireVsCodeApi function is provided by VS Code in the webview environment.
// We need to declare it to make TypeScript happy.
declare function acquireVsCodeApi(): VsCodeApi;

export const vscode = acquireVsCodeApi();
