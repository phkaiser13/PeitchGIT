import * as vscode from 'vscode';
import { IGitAdapter } from '../git/types';
import { Logger } from '../utils/logger';
import { getAvailableProviders, getAdapter } from '../ia/adapterFactory';
import { setApiKey, getApiKey } from '../ia/keychain';
import { getSettings } from '../utils/settings';

// AI Prompt Templates are now embedded in the code
const commitGeneratorTemplate = `Generate a few concise and conventional commit messages for the following git diff.
Provide between 2 and 5 suggestions. Each suggestion should be a single line, suitable for a commit message title.
Do not add any other text, explanation, or formatting like markdown. Just return the commit messages.

Here is the diff:
---
{{diff}}
---`;

const changelogGeneratorTemplate = `Generate a changelog in Markdown format from the following list of git commit messages.
Categorize the changes into "Features", "Fixes", and "Chores" sections.
If a category has no relevant commits, omit the category heading.
The output should be clean markdown, ready to be used in release notes.

Here are the commit messages:
---
{{commits}}
---`;


function getWorkspacePath(): string | undefined {
    return vscode.workspace.workspaceFolders?.[0].uri.fsPath;
}

function redactSensitiveInfo(text: string): string {
    // Basic redaction for logging purposes
    return text.replace(/(api_key|token|secret)\s*=\s*['"]?[^'"]+['"]?/gi, '$1="[REDACTED]"');
}

export function registerAICommands(context: vscode.extensionContext, gitAdapter: IGitAdapter): vscode.Disposable[] {
    const subscriptions: vscode.Disposable[] = [];

    subscriptions.push(vscode.commands.registerCommand('peitch.addAIProvider', async () => {
        try {
            const providers = getAvailableProviders();
            const selectedProvider = await vscode.window.showQuickPick(
                providers.map(p => ({ label: p.name, id: p.id })),
                { placeHolder: 'Selecione um provedor de IA para configurar' }
            );

            if (!selectedProvider) return;

            const apiKey = await vscode.window.showInputBox({
                prompt: `Digite a chave de API para ${selectedProvider.label}`,
                password: true,
                ignoreFocusOut: true,
            });

            if (apiKey) {
                await setApiKey(selectedProvider.id, apiKey);
                vscode.window.showInformationMessage(`Chave de API para ${selectedProvider.label} salva com sucesso.`);
                Logger.log(`API Key set for provider: ${selectedProvider.id}`);
            }
        } catch (error) {
            Logger.showError('Falha ao adicionar provedor de IA', error);
        }
    }));

    subscriptions.push(vscode.commands.registerCommand('peitch.generateCommitWithAI', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('Nenhum repositório encontrado.');
            return;
        }

        try {
            const stagedDiff = await gitAdapter.getStagedDiff(repoPath);
            if (!stagedDiff) {
                vscode.window.showInformationMessage('Nenhum arquivo no stage. Adicione arquivos para gerar uma mensagem de commit.');
                return;
            }
            
            const providers = getAvailableProviders();
            const selectedProvider = await vscode.window.showQuickPick(
                providers.map(p => ({ label: p.name, id: p.id })),
                { placeHolder: 'Selecione o provedor de IA para gerar a mensagem de commit' }
            );
            if (!selectedProvider) return;

            const adapter = getAdapter(selectedProvider.id);
            if (!adapter) {
                Logger.showError(`Não foi possível encontrar o adaptador para ${selectedProvider.label}`);
                return;
            }
            
            const apiKey = await getApiKey(selectedProvider.id);
            if (!apiKey && selectedProvider.id !== 'local') {
                 vscode.window.showErrorMessage(`Chave de API para ${selectedProvider.label} não configurada. Execute 'Peitch: Add AI Provider Configuration'.`);
                 return;
            }

            await vscode.window.withProgress({
                location: vscode.ProgressLocation.Notification,
                title: `Gerando mensagem de commit com ${selectedProvider.label}...`,
                cancellable: false
            }, async () => {
                const prompt = commitGeneratorTemplate.replace('{{diff}}', stagedDiff);
                
                const redactedPrompt = redactSensitiveInfo(prompt);
                Logger.log(`Generating commit with prompt: ${redactedPrompt}`);

                const suggestions = await adapter.generate(redactedPrompt, apiKey || '');

                if (!suggestions || suggestions.length === 0) {
                    vscode.window.showWarningMessage('A IA não retornou nenhuma sugestão.');
                    return;
                }

                const items = suggestions.map(s => ({ label: s.trim() })).filter(s => s.label);
                const editOption = { label: '$(edit) Editar mensagem manualmente...', description: 'Digite uma mensagem de commit personalizada' };
                const selected = await vscode.window.showQuickPick([editOption, ...items], {
                    placeHolder: 'Selecione uma mensagem de commit gerada ou edite manualmente'
                });

                if (!selected) return;

                let commitMessage = selected.label;
                if (selected.label === editOption.label) {
                    const customMessage = await vscode.window.showInputBox({ prompt: 'Digite a mensagem do commit' });
                    if (!customMessage) return;
                    commitMessage = customMessage;
                }

                const commitSha = await gitAdapter.commit(repoPath, commitMessage);
                const shortSha = commitSha.substring(0, 7);
                vscode.window.showInformationMessage(`Commit realizado com sucesso: ${shortSha}`);
                Logger.log(`Committed with AI message "${commitMessage}" (sha: ${commitSha})`);
            });

        } catch (error) {
            Logger.showError('Falha ao gerar mensagem de commit com IA ou ao commitar', error);
        }
    }));

    subscriptions.push(vscode.commands.registerCommand('peitch.generatePR', async () => {
        vscode.window.showInformationMessage('Gerar Descrição de PR ainda não foi totalmente implementado.');
        Logger.log('Comando Gerar Descrição de PR executado (stub).');
    }));

    subscriptions.push(vscode.commands.registerCommand('peitch.createRelease', async () => {
        const repoPath = getWorkspacePath();
        if (!repoPath) {
            Logger.showError('Nenhum repositório encontrado.');
            return;
        }

        try {
            const settings = getSettings();
            if (!settings.allowSendToIA) {
                Logger.showError('Recursos de IA estão desabilitados nas configurações da Peitch.');
                return;
            }

            const tags = await gitAdapter.getTags(repoPath);
            const branches = await gitAdapter.getBranches(repoPath);
            const refs = [...tags, ...branches.map(b => `origin/${b}`), ...branches];
            
            const fromRef = await vscode.window.showQuickPick(refs, { placeHolder: 'Selecione a tag/branch inicial para o changelog (ex: última release)' });
            if (!fromRef) return;

            const toRefItems = [settings.defaultBranch, ...refs.filter(r => r !== settings.defaultBranch)];
            const toRef = await vscode.window.showQuickPick(toRefItems, { 
                placeHolder: 'Selecione a tag/branch final (ex: main)', 
                canPickMany: false,
            });
            if (!toRef) return;

            const newTagName = await vscode.window.showInputBox({ prompt: 'Digite o nome da nova tag da release (ex: v1.2.0)' });
            if (!newTagName) return;

            const providers = getAvailableProviders();
            const selectedProvider = await vscode.window.showQuickPick(providers.map(p => ({ label: p.name, id: p.id })), { placeHolder: 'Selecione o provedor de IA para gerar o changelog' });
            if (!selectedProvider) return;
            
            const adapter = getAdapter(selectedProvider.id);
            if (!adapter) { Logger.showError(`Não foi possível encontrar o adaptador para ${selectedProvider.label}`); return; }

            const apiKey = await getApiKey(selectedProvider.id);
            if (!apiKey && selectedProvider.id !== 'local') { vscode.window.showErrorMessage(`Chave de API para ${selectedProvider.label} não configurada.`); return; }

            await vscode.window.withProgress({
                location: vscode.ProgressLocation.Notification,
                title: `Gerando changelog com ${selectedProvider.label}...`,
                cancellable: false
            }, async (progress) => {
                progress.report({ message: 'Buscando commits...' });
                const commits = await gitAdapter.getCommitsBetween(repoPath, fromRef, toRef);
                if (commits.length === 0) {
                    vscode.window.showWarningMessage(`Nenhum commit encontrado entre ${fromRef} e ${toRef}.`);
                    return;
                }
                const commitMessages = commits.map(c => c.message).join('\n');

                progress.report({ message: 'Gerando changelog com IA...' });
                const prompt = changelogGeneratorTemplate.replace('{{commits}}', commitMessages);

                Logger.log(`Generating changelog with prompt for commits between ${fromRef} and ${toRef}`);
                const suggestions = await adapter.generate(prompt, apiKey || '');
                const changelog = suggestions[0] || 'A geração do changelog falhou.';

                progress.report({ message: 'Criando tag...' });
                await gitAdapter.createAnnotatedTag(repoPath, newTagName, `Release ${newTagName}\n\n${changelog}`);
                
                Logger.log(`Created tag ${newTagName}`);
                vscode.window.showInformationMessage(`Tag de release ${newTagName} criada com sucesso.`);
                vscode.commands.executeCommand('peitch.refreshTreeView');

                const pushTag = await vscode.window.showQuickPick(['Sim', 'Não'], { placeHolder: `Fazer push da tag "${newTagName}" para a origin?`});
                if (pushTag === 'Sim') {
                    progress.report({ message: `Fazendo push da tag ${newTagName}...` });
                    await gitAdapter.pushToRemote(repoPath, 'origin', newTagName, { tags: true });
                    vscode.window.showInformationMessage(`Tag ${newTagName} enviada para a origin.`);
                }
            });
        } catch (error) {
            Logger.showError('Falha ao criar release', error);
        }
    }));


    return subscriptions;
}