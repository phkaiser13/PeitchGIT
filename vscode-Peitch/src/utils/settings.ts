import * as vscode from 'vscode';

export interface DeployTarget {
    name: string;
    type: 'ssh' | 's3' | 'k8s' | 'webhook';
    endpoint: string;
}

export interface PeitchSettings {
    defaultBranch: string;
    autoFetchIntervalMin: number;
    allowSendToIA: boolean;
    mirrors: string[];
    deployTargets: DeployTarget[];
}

export function getSettings(): PeitchSettings {
    const config = vscode.workspace.getConfiguration('peitch');
    return {
        defaultBranch: config.get<string>('defaultBranch') || 'main',
        autoFetchIntervalMin: config.get<number>('autoFetchIntervalMin') || 0,
        allowSendToIA: config.get<boolean>('allowSendToIA') ?? true,
        mirrors: config.get<string[]>('mirrors') || [],
        deployTargets: config.get<DeployTarget[]>('deployTargets') || [],
    };
}
