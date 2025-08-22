import { describe, it, expect, jest, beforeEach } from '@jest/globals';
import { registerAICommands } from '../src/commands/aiCommands';
import { IGitAdapter } from '../src/git/types';
import * as vscode from 'vscode';
import { getApiKey, setApiKey } from '../src/ia/keychain';
import { getAdapter } from '../src/ia/adapterFactory';

// Mock vscode APIs
jest.mock('vscode', () => ({
    window: {
        showQuickPick: jest.fn(),
        showInputBox: jest.fn(),
        withProgress: jest.fn((options: any, task: (progress: any, token: any) => Promise<any>) => task({ report: jest.fn() }, { onCancellationRequested: jest.fn(), isCancellationRequested: false })),
        showInformationMessage: jest.fn(),
        showErrorMessage: jest.fn(),
    },
    ProgressLocation: {
        Notification: 15
    },
    commands: {
        registerCommand: jest.fn((name, handler) => ({ name, handler, dispose: jest.fn() })),
        executeCommand: jest.fn(),
    },
    workspace: {
        workspaceFolders: [{ uri: { fsPath: '/fake/repo' } }],
        getConfiguration: () => ({
            get: (key: string) => {
                if (key === 'allowSendToIA') return true;
                if (key === 'defaultBranch') return 'main';
                return undefined;
            }
        })
    },
}), { virtual: true });

// Mock other dependencies
jest.mock('../src/ia/keychain');
jest.mock('../src/ia/adapterFactory');

const mockedVscode = vscode as jest.Mocked<typeof vscode>;
const mockedGetApiKey = getApiKey as jest.Mock;
const mockedGetAdapter = getAdapter as jest.Mock;

describe('AI Command: createRelease', () => {
    let mockGitAdapter: jest.Mocked<IGitAdapter>;
    let createReleaseHandler: (...args: any[]) => any;

    beforeEach(() => {
        jest.clearAllMocks();

        mockGitAdapter = {
            getStatus: jest.fn(),
            getStagedFiles: jest.fn(),
            getBranches: jest.fn().mockResolvedValue(['main', 'develop']),
            getCommitLog: jest.fn(),
            getFilesForBranch: jest.fn(),
            stageFiles: jest.fn(),
            unstageFiles: jest.fn(),
            commit: jest.fn(),
            push: jest.fn(),
            fetch: jest.fn(),
            checkout: jest.fn(),
            getStagedDiff: jest.fn(),
            getDiff: jest.fn(),
            getRemotes: jest.fn(),
            addRemote: jest.fn(),
            pushToRemote: jest.fn().mockResolvedValue(undefined),
            getTags: jest.fn().mockResolvedValue(['v1.0.0']),
            getCommitsBetween: jest.fn().mockResolvedValue([{ sha: 'abc', message: 'feat: new thing' }]),
            createAnnotatedTag: jest.fn().mockResolvedValue(undefined),
            getWorktrees: jest.fn(),
            addWorktree: jest.fn(),
            removeWorktree: jest.fn(),
        };
        
        mockedGetApiKey.mockResolvedValue('fake-api-key');
        
        const mockAIAdapter = {
            generate: jest.fn().mockResolvedValue(['## Changelog\n* feat: new thing']),
        };
        mockedGetAdapter.mockReturnValue(mockAIAdapter as any);

        // Register commands and get the handler
        const commands = registerAICommands({ extensionPath: '/fake/path' } as any, mockGitAdapter);
        const releaseCommand = commands.find(c => (c as any).name === 'peitch.createRelease');
        if (!releaseCommand) {
            throw new Error('createRelease command not found');
        }
        createReleaseHandler = (releaseCommand as any).handler;

    });

    it('should execute the full release flow successfully and push tag', async () => {
        // Mock user input
        (mockedVscode.window.showQuickPick as jest.Mock)
            .mockResolvedValueOnce('v1.0.0') // fromRef
            .mockResolvedValueOnce('main') // toRef
            .mockResolvedValueOnce({ label: 'Google Gemini', id: 'gemini' }) // AI provider
            .mockResolvedValueOnce('Sim'); // Push tag
        (mockedVscode.window.showInputBox as jest.Mock).mockResolvedValueOnce('v1.1.0'); // New tag name

        await createReleaseHandler();

        // Verify the flow
        expect(mockGitAdapter.getTags).toHaveBeenCalled();
        expect(mockGitAdapter.getBranches).toHaveBeenCalled();
        expect(mockedVscode.window.showQuickPick).toHaveBeenCalledWith(expect.any(Array), { placeHolder: 'Selecione a tag/branch inicial para o changelog (ex: última release)' });
        expect(mockGitAdapter.getCommitsBetween).toHaveBeenCalledWith('/fake/repo', 'v1.0.0', 'main');
        expect(mockedGetAdapter).toHaveBeenCalledWith('gemini');
        expect(getAdapter('gemini')?.generate).toHaveBeenCalledWith(expect.stringContaining('feat: new thing'), 'fake-api-key');
        expect(mockGitAdapter.createAnnotatedTag).toHaveBeenCalledWith('/fake/repo', 'v1.1.0', expect.stringContaining('## Changelog'));
        expect(mockedVscode.window.showInformationMessage).toHaveBeenCalledWith('Tag de release v1.1.0 criada com sucesso.');
        expect(mockGitAdapter.pushToRemote).toHaveBeenCalledWith('/fake/repo', 'origin', 'v1.1.0', { tags: true });
    });

    it('should not push the tag if user selects "Não"', async () => {
        // Mock user input
        (mockedVscode.window.showQuickPick as jest.Mock)
            .mockResolvedValueOnce('v1.0.0') // fromRef
            .mockResolvedValueOnce('main') // toRef
            .mockResolvedValueOnce({ label: 'Google Gemini', id: 'gemini' }) // AI provider
            .mockResolvedValueOnce('Não'); // Push tag
        (mockedVscode.window.showInputBox as jest.Mock).mockResolvedValueOnce('v1.1.0'); // New tag name

        await createReleaseHandler();

        expect(mockGitAdapter.createAnnotatedTag).toHaveBeenCalled();
        expect(mockGitAdapter.pushToRemote).not.toHaveBeenCalled();
    });
});