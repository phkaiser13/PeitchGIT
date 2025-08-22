import { describe, it, expect, beforeEach, jest } from '@jest/globals';
import { GitAdapter } from '../src/git/gitAdapter';
import { exec, ExecException } from 'child_process';
import * as path from 'path';

// Mock child_process.exec
jest.mock('child_process', () => ({
    exec: jest.fn(),
}));
const mockedExec = exec as unknown as jest.Mock;

describe('GitAdapter', () => {
    let adapter: GitAdapter;
    const repoPath = '/fake/repo';

    beforeEach(() => {
        adapter = new GitAdapter();
        mockedExec.mockReset();
    });

    const mockExec = (commandMap: { [key: string]: string }) => {
        mockedExec.mockImplementation((command: string, options: any, callback: (error: ExecException | null, stdout: string, stderr: string) => void) => {
            // Find a matching command, allowing for variations in quoting etc.
            const commandKey = Object.keys(commandMap).find(key => command.includes(key));
            const stdout = commandKey ? commandMap[commandKey] : '';
            callback(null, stdout, '');
        });
    };

    it('should parse git status correctly', async () => {
        mockExec({
            'rev-parse --abbrev-ref HEAD': 'main',
            'rev-list --left-right --count': '1\t2', // 1 ahead, 2 behind
            'status --porcelain': ` M src/git/gitAdapter.ts\nA  src/git/types.ts\n?? new-file.txt\nUU conflict.json`,
            'rev-parse --abbrev-ref --symbolic-full-name @{u}': 'origin/main',
        });
        
        const status = await adapter.getStatus(repoPath);

        expect(status.branch).toBe('main');
        expect(status.ahead).toBe(1);
        expect(status.behind).toBe(2);
        expect(status.staged).toEqual(['src/git/types.ts', 'conflict.json']);
        expect(status.unstaged).toEqual(['src/git/gitAdapter.ts', 'conflict.json']);
        expect(status.untracked).toEqual(['new-file.txt']);
        expect(status.conflicts).toEqual(['conflict.json']);
    });

    it('should get remotes correctly', async () => {
        mockExec({
            'remote -v': `origin\tgit@github.com:user/repo.git (fetch)\norigin\tgit@github.com:user/repo.git (push)\nupstream\thttps://github.com/upstream/repo.git (fetch)\nupstream\thttps://github.com/upstream/repo.git (push)`
        });
        const remotes = await adapter.getRemotes(repoPath);
        expect(remotes).toEqual([
            { name: 'origin', url: 'git@github.com:user/repo.git' },
            { name: 'upstream', url: 'https://github.com/upstream/repo.git' },
        ]);
    });

    it('should get tags correctly', async () => {
        mockExec({
            'tag -l --sort=-creatordate': `v1.1.0\nv1.0.0\nv0.1.0`
        });
        const tags = await adapter.getTags(repoPath);
        expect(tags).toEqual(['v1.1.0', 'v1.0.0', 'v0.1.0']);
    });
    
    it('should get worktrees correctly', async () => {
        const mainWorktreePath = repoPath;
        const featureWorktreePath = path.resolve(repoPath, '../repo-feature-branch');
        
        mockExec({
            'worktree list': `${featureWorktreePath}  abcdef1 [feature-branch]`,
            'rev-parse --git-dir': `${mainWorktreePath}/.git`,
            'rev-parse --abbrev-ref HEAD': 'main',
        });

        const worktrees = await adapter.getWorktrees(repoPath);

        // This test relies on path resolution, so we check properties
        expect(worktrees.length).toBe(2);
        expect(worktrees[0]).toMatchObject({
            path: mainWorktreePath,
            branch: 'main',
            isCurrent: true,
        });
        expect(worktrees[1]).toMatchObject({
            path: featureWorktreePath,
            branch: 'feature-branch',
            isCurrent: false,
        });
    });

    it('should create an annotated tag', async () => {
        mockExec({}); // No stdout needed, just verifying the command
        const tagName = 'v1.2.3';
        const message = 'Release version 1.2.3';
        await adapter.createAnnotatedTag(repoPath, tagName, message);
        
        expect(mockedExec).toHaveBeenCalledWith(
            `git tag -a "${tagName}" -m "${message}"`,
            expect.any(Object),
            expect.any(Function)
        );
    });
});