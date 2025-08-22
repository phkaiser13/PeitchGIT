import { GitStatus, Commit } from "../git/types";

// Messages from Webview to Extension
export type WebviewToExtensionMessage =
    | { type: 'ready' }
    | { type: 'getStatus' }
    | { type: 'getCommitLog' }
    | { type: 'getFilesForBranch'; payload: { branch: string } }
    | { type: 'stageFiles'; payload: { files: string[] } }
    | { type: 'unstageFiles'; payload: { files: string[] } }
    | { type: 'commit'; payload: { message: string } }
    | { type: 'push' }
    | { type: 'fetch' }
    | { type: 'checkout'; payload: { branch: string } }
    | { type: 'showError'; payload: { message: string } };

// Messages from Extension to Webview
export type ExtensionToWebviewMessage =
    | { type: 'status'; payload: GitStatus }
    | { type: 'commitLog'; payload: Commit[] }
    | { type: 'branchFiles'; payload: { branch: string, files: string[] } }
    | { type: 'showBranch'; payload: { branch: string } }
    | { type: 'operationSuccess'; payload: { message: string } }
    | { type: 'error'; payload: { message: string } };