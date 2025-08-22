import { exec } from 'child_process';
import { promisify } from 'util';
import { IGitAdapter, GitStatus, Commit, Remote, PushOptions, Worktree } from './types';
import * as path from 'path';

const execAsync = promisify(exec);

export class GitAdapter implements IGitAdapter {
    private async execute(repoPath: string, command: string): Promise<string> {
        try {
            // maxBuffer default is 1MB, increase to handle potentially large git outputs
            const { stdout } = await execAsync(`git ${command}`, { cwd: repoPath, maxBuffer: 1024 * 5000 });
            return stdout.trim();
        } catch (error: any) {
            console.error(`Error executing "git ${command}":`, error);
            throw new Error(error.stderr || error.message);
        }
    }

    public async getStatus(repoPath: string): Promise<GitStatus> {
        const status: GitStatus = {
            branch: '',
            staged: [],
            unstaged: [],
            untracked: [],
            conflicts: [],
        };

        try {
            status.branch = await this.execute(repoPath, 'rev-parse --abbrev-ref HEAD');
        } catch (e) {
            status.branch = 'HEAD detached';
        }

        if (status.branch !== 'HEAD detached') {
            try {
                const upstream = await this.execute(repoPath, 'rev-parse --abbrev-ref --symbolic-full-name @{u}');
                if (upstream) {
                    const aheadBehind = await this.execute(repoPath, 'rev-list --left-right --count HEAD...' + upstream);
                    const [ahead, behind] = aheadBehind.split('\t').map(Number);
                    status.ahead = ahead;
                    status.behind = behind;
                }
            } catch (e) {
                // No upstream branch, ignore
            }
        }
        
        const output = await this.execute(repoPath, 'status --porcelain');
        if (!output) {
            return status;
        }

        const lines = output.split('\n').filter(line => line.length > 0);
        for (const line of lines) {
            const statusChars = line.substring(0, 2);
            const filePath = line.substring(3);

            if (statusChars === '??') {
                status.untracked.push(filePath);
            } else if (statusChars === 'UU') {
                status.conflicts.push(filePath);
            } else {
                // Staged files have a status character in the first column
                if (statusChars[0].trim().length > 0) {
                     status.staged.push(filePath);
                }
                // Unstaged files have a status character in the second column
                if (statusChars[1].trim().length > 0) {
                     status.unstaged.push(filePath);
                }
            }
        }

        return status;
    }

    public async getStagedFiles(repoPath: string): Promise<string[]> {
        const output = await this.execute(repoPath, 'diff --name-only --cached');
        return output.split('\n').filter(Boolean);
    }

    public async getBranches(repoPath: string): Promise<string[]> {
        const output = await this.execute(repoPath, 'branch');
        return output.split('\n').map(b => b.trim().replace('* ', ''));
    }

    public async getCommitLog(repoPath: string, count: number = 20): Promise<Commit[]> {
        const format = `"%H|%s"`; // SHA|subject
        const output = await this.execute(repoPath, `log -n ${count} --pretty=format:${format}`);
        if (!output) return [];
        return output.split('\n').map(line => {
            const [sha, message] = line.slice(1, -1).split('|');
            return { sha, message };
        });
    }

    public async getFilesForBranch(repoPath: string, branch: string): Promise<string[]> {
        const output = await this.execute(repoPath, `ls-tree -r --name-only ${branch}`);
        return output.split('\n').filter(Boolean);
    }

    public async stageFiles(repoPath: string, files: string[]): Promise<void> {
        if (files.length === 0) return;
        const fileList = files.map(f => `"${f}"`).join(' ');
        await this.execute(repoPath, `add ${fileList}`);
    }

    public async unstageFiles(repoPath: string, files: string[]): Promise<void> {
        if (files.length === 0) return;
        const fileList = files.map(f => `"${f}"`).join(' ');
        await this.execute(repoPath, `reset HEAD -- ${fileList}`);
    }

    public async commit(repoPath: string, message: string): Promise<string> {
        const sanitizedMessage = message.replace(/"/g, '\\"');
        await this.execute(repoPath, `commit -m "${sanitizedMessage}"`);
        return this.execute(repoPath, 'rev-parse HEAD');
    }

    public async push(repoPath: string, remote: string = 'origin', branch?: string): Promise<void> {
        const currentBranch = branch || (await this.getStatus(repoPath)).branch;
        await this.pushToRemote(repoPath, remote, currentBranch);
    }

    public async fetch(repoPath: string, remote: string = 'origin'): Promise<void> {
        await this.execute(repoPath, `fetch ${remote}`);
    }

    public async checkout(repoPath: string, branch: string, create: boolean = false): Promise<void> {
        const createFlag = create ? '-b' : '';
        await this.execute(repoPath, `checkout ${createFlag} ${branch}`);
    }

    public async getStagedDiff(repoPath: string): Promise<string> {
        return this.execute(repoPath, 'diff --staged');
    }

    public async getDiff(repoPath: string, from: string, to: string): Promise<string> {
        return this.execute(repoPath, `diff ${from}...${to}`);
    }

    // New advanced methods

    public async getRemotes(repoPath: string): Promise<Remote[]> {
        const output = await this.execute(repoPath, 'remote -v');
        if (!output) return [];
        const lines = output.split('\n');
        const remotes: { [name: string]: Remote } = {};
        lines.forEach(line => {
            const parts = line.split(/\s+/);
            if (parts.length >= 2 && !remotes[parts[0]]) {
                remotes[parts[0]] = { name: parts[0], url: parts[1] };
            }
        });
        return Object.values(remotes);
    }

    public async addRemote(repoPath: string, name: string, url: string): Promise<void> {
        await this.execute(repoPath, `remote add "${name}" "${url}"`);
    }
    
    public async pushToRemote(repoPath: string, remote: string, branch: string, options: PushOptions = {}): Promise<void> {
        let command = 'push';
        if (options.force) command += ' --force';
        if (options.mirror) command += ' --mirror';
        if (options.all) command += ' --all';
        if (options.tags) command += ' --tags';
        command += ` ${remote}`;
        if (!options.mirror && !options.all) {
            command += ` ${branch}`;
        }
        await this.execute(repoPath, command);
    }

    public async getTags(repoPath: string): Promise<string[]> {
        const output = await this.execute(repoPath, 'tag -l --sort=-creatordate');
        return output.split('\n').filter(Boolean);
    }

    public async getCommitsBetween(repoPath: string, from: string, to: string): Promise<Commit[]> {
        const format = `"%H|%s"`;
        const output = await this.execute(repoPath, `log ${from}..${to} --pretty=format:${format}`);
        if (!output) return [];
        return output.split('\n').map(line => {
            const [sha, message] = line.slice(1, -1).split('|');
            return { sha, message };
        });
    }

    public async createAnnotatedTag(repoPath: string, tagName: string, message: string): Promise<void> {
        const sanitizedMessage = message.replace(/"/g, '\\"');
        await this.execute(repoPath, `tag -a "${tagName}" -m "${sanitizedMessage}"`);
    }

    public async getWorktrees(repoPath: string): Promise<Worktree[]> {
        const output = await this.execute(repoPath, 'worktree list');
        if (!output) return [];
        
        const mainWorktreePath = (await this.execute(repoPath, 'rev-parse --git-dir')).replace('/.git', '');
        
        const worktrees: Worktree[] = [];
        
        const lines = output.split('\n').filter(Boolean);
        lines.forEach(line => {
            const parts = line.split(/\s+/);
            const currentPath = parts[0];
            const branch = parts[2].replace(/[\[\]]/g, '');
            worktrees.push({
                path: currentPath,
                branch: branch,
                isCurrent: path.resolve(currentPath) === path.resolve(repoPath),
            });
        });

        // Ensure main worktree is included if not listed explicitly
        if (!worktrees.some(w => path.resolve(w.path) === path.resolve(mainWorktreePath))) {
            const branch = await this.execute(mainWorktreePath, 'rev-parse --abbrev-ref HEAD');
             worktrees.unshift({
                path: mainWorktreePath,
                branch: branch,
                isCurrent: path.resolve(mainWorktreePath) === path.resolve(repoPath),
            });
        }
        
        return worktrees;
    }
    
    public async addWorktree(repoPath: string, path: string, branch: string, newBranch: boolean = false): Promise<void> {
        const newBranchFlag = newBranch ? '-b' : '';
        await this.execute(repoPath, `worktree add ${newBranchFlag} "${path}" "${branch}"`);
    }

    public async removeWorktree(repoPath: string, path: string): Promise<void> {
        await this.execute(repoPath, `worktree remove "${path}"`);
    }
}
