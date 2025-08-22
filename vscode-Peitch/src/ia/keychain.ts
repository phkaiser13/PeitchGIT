import * as vscode from 'vscode';

let secretStorage: any;

export function initializeKeychain(context: vscode.ExtensionContext) {
    secretStorage = (context as any).secrets;
}

const KEY_PREFIX = 'peitch.aiProvider.apiKey.';

export async function setApiKey(providerId: string, apiKey: string): Promise<void> {
    if (!secretStorage) {
        throw new Error('Keychain not initialized.');
    }
    await secretStorage.store(`${KEY_PREFIX}${providerId}`, apiKey);
}

export async function getApiKey(providerId: string): Promise<string | undefined> {
    if (!secretStorage) {
        throw new Error('Keychain not initialized.');
    }
    return await secretStorage.get(`${KEY_PREFIX}${providerId}`);
}