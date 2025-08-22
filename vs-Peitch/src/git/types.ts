export interface GitStatus {
  branch: string;
  ahead?: number;
  behind?: number;
  staged: string[];
  unstaged: string[];
  untracked: string[];
  conflicts: string[];
}

export interface Commit {
    sha: string;
    message: string;
}

export interface Remote {
    name: string;
    url: string;
}

export interface Worktree {
    path: string;
    branch: string;
    isCurrent: boolean;
}

export interface PushOptions {
    force?: boolean;
    mirror?: boolean;
    all?: boolean;
    tags?: boolean;
}

export interface IGitAdapter {
  getStatus(repoPath: string): Promise<GitStatus>;
  getStagedFiles(repoPath: string): Promise<string[]>;
  getBranches(repoPath: string): Promise<string[]>;
  getCommitLog(repoPath: string, count?: number): Promise<Commit[]>;
  getFilesForBranch(repoPath: string, branch: string): Promise<string[]>;
  stageFiles(repoPath: string, files: string[]): Promise<void>;
  unstageFiles(repoPath: string, files: string[]): Promise<void>;
  commit(repoPath: string, message: string): Promise<string>; // returns commit sha
  push(repoPath: string, remote?: string, branch?: string): Promise<void>;
  fetch(repoPath: string, remote?: string): Promise<void>;
  checkout(repoPath: string, branch: string, create?: boolean): Promise<void>;
  getStagedDiff(repoPath: string): Promise<string>;
  getDiff(repoPath: string, from: string, to: string): Promise<string>;

  // New advanced methods
  getRemotes(repoPath: string): Promise<Remote[]>;
  addRemote(repoPath: string, name: string, url: string): Promise<void>;
  pushToRemote(repoPath: string, remote: string, branch: string, options?: PushOptions): Promise<void>;
  getTags(repoPath: string): Promise<string[]>;
  getCommitsBetween(repoPath: string, from: string, to: string): Promise<Commit[]>;
  createAnnotatedTag(repoPath: string, tagName: string, message: string): Promise<void>;
  getWorktrees(repoPath: string): Promise<Worktree[]>;
  addWorktree(repoPath: string, path: string, branch: string, newBranch?: boolean): Promise<void>;
  removeWorktree(repoPath: string, path: string): Promise<void>;
}
