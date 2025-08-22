import * as vscode from 'vscode';
import { Logger } from '../utils/logger';
import { getSettings } from '../utils/settings';

export function registerDeployCommands(): vscode.Disposable[] {
    const subscriptions: vscode.Disposable[] = [];

    subscriptions.push(vscode.commands.registerCommand('peitch.deploy', async () => {
        const settings = getSettings();
        const targets = settings.deployTargets;

        if (!targets || targets.length === 0) {
            Logger.showError("No deployment targets configured in 'peitch.deployTargets' setting.");
            return;
        }

        const targetNames = targets.map(t => t.name);
        const selectedTargetName = await vscode.window.showQuickPick(targetNames, {
            placeHolder: 'Select a deployment target'
        });

        if (!selectedTargetName) return;

        const target = targets.find(t => t.name === selectedTargetName);
        if (target) {
            vscode.window.showInformationMessage(`Simulating deployment to '${target.name}' (${target.type})...`);
            Logger.log(`[DEPLOY STUB] Deploying to ${target.name} via ${target.type} at ${target.endpoint}`);
            // This is a stub. Real implementation would go here.
            setTimeout(() => {
                vscode.window.showInformationMessage(`Mock deployment to '${target.name}' completed.`);
            }, 2000);
        }
    }));

    return subscriptions;
}
