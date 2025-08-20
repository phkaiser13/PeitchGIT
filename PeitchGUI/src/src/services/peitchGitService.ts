import { invoke } from '../bridge';
import { phgit } from './generated/phgit';
import type { Repository, WorkspaceStatus, Commit, Branch, FileChange, WorkspaceFileChange, UserSettings } from '../types';

declare global {
    interface Window {
        peitch?: {
            listRepos: () => Promise<{ name: string; path: string; }[]>;
        };
    }
}

// --- Mappers (Protobuf -> Internal Types) ---

function mapProtoCommitToCommit(protoCommit: phgit.Commit): Commit {
    return {
        sha: protoCommit.sha,
        shortSha: protoCommit.shortSha,
        message: protoCommit.message,
        author: {
            name: protoCommit.author?.name || 'Unknown',
            email: protoCommit.author?.email || 'unknown@example.com',
        },
        date: new Date(protoCommit.dateTimestamp * 1000).toISOString(),
        parents: protoCommit.parents || [],
        files: [], // The proto doesn't include files per commit; this would require a separate, more detailed query.
        branch: protoCommit.branch,
    };
}

function mapProtoBranchToBranch(protoBranch: phgit.Branch): Branch {
    return {
        name: protoBranch.name,
        isRemote: protoBranch.isRemote,
        sha: protoBranch.sha,
    };
}

function mapProtoFileChangeToWorkspaceFileChange(protoFile: phgit.WorkspaceFileChange, staged: boolean): WorkspaceFileChange {
    const statusMap: { [key: string]: FileChange['status'] } = {
        added: 'added',
        deleted: 'deleted',
        modified: 'modified',
        renamed: 'renamed',
    };
    return {
        id: `${staged ? 's' : 'u'}-${protoFile.path}`,
        path: protoFile.path,
        status: statusMap[protoFile.status] || 'modified',
        diff: protoFile.diff,
        staged: staged,
    };
}

// --- Service Functions ---

// This function is kept from the old service, as it's a discovery mechanism
// that might be separate from the main PeitchGIT Protobuf API.
export const getRepositories = async (): Promise<Omit<Repository, 'commits' | 'branches' | 'tags' | 'stashes' | 'remotes'>[]> => {
    if (!window.peitch || !window.peitch.listRepos) {
        console.error("Repository host environment is not available.");
        return [];
    }
    const repos = await window.peitch.listRepos();
    return repos.map(r => ({ id: r.path, name: r.name, path: r.path }));
};

// This function loads the essential data for a repository view.
export const getRepositoryById = async (path: string): Promise<Pick<Repository, 'commits' | 'branches' | 'tags' | 'stashes' | 'remotes'>> => {
    const request = phgit.RepoRequest.create({ repoPath: path });
    const payload = phgit.RepoRequest.encode(request).finish();
    const responsePayload = await invoke('load_repository', payload);
    const decoded = phgit.LoadRepositoryResponse.decode(responsePayload);

    // The proto only specifies commits and branches. We return empty for the rest.
    // A real implementation would expand the `LoadRepositoryResponse` proto message.
    return {
        commits: (decoded.commits || []).map(mapProtoCommitToCommit),
        branches: (decoded.branches || []).map(mapProtoBranchToBranch),
        tags: [],
        stashes: [],
        remotes: [],
    };
};

export const getWorkspaceStatus = async (repoId: string): Promise<WorkspaceStatus> => {
    const request = phgit.RepoRequest.create({ repoPath: repoId });
    const payload = phgit.RepoRequest.encode(request).finish();
    const responsePayload = await invoke('get_workspace_status', payload);
    const decoded = phgit.WorkspaceStatusResponse.decode(responsePayload);

    return {
        stagedFiles: (decoded.stagedFiles || []).map(f => mapProtoFileChangeToWorkspaceFileChange(f, true)),
        unstagedFiles: (decoded.unstagedFiles || []).map(f => mapProtoFileChangeToWorkspaceFileChange(f, false)),
    };
};

export const stageFile = async (repoId: string, filePath: string): Promise<void> => {
    const request = phgit.FileRequest.create({ repoPath: repoId, filePath });
    const payload = phgit.FileRequest.encode(request).finish();
    await invoke('stage_file', payload);
};

export const unstageFile = async (repoId: string, filePath: string): Promise<void> => {
    const request = phgit.FileRequest.create({ repoPath: repoId, filePath });
    const payload = phgit.FileRequest.encode(request).finish();
    await invoke('unstage_file', payload);
};

export const stageAll = async (repoId: string): Promise<void> => {
    const request = phgit.RepoRequest.create({ repoPath: repoId });
    const payload = phgit.RepoRequest.encode(request).finish();
    await invoke('stage_all', payload);
};

export const unstageAll = async (repoId: string): Promise<void> => {
    const request = phgit.RepoRequest.create({ repoPath: repoId });
    const payload = phgit.RepoRequest.encode(request).finish();
    await invoke('unstage_all', payload);
};

export const commit = async (repoId: string, message: string, authorName: string, authorEmail: string): Promise<void> => {
    const request = phgit.CommitRequest.create({ repoPath: repoId, message, authorName, authorEmail });
    const payload = phgit.CommitRequest.encode(request).finish();
    await invoke('commit', payload);
};

export const checkoutBranch = async (repoId: string, branchName: string): Promise<void> => {
    const request = phgit.BranchRequest.create({ repoPath: repoId, branchName, baseSha: '' }); // baseSha not needed for checkout
    const payload = phgit.BranchRequest.encode(request).finish();
    await invoke('checkout_branch', payload);
};

export const createBranch = async (repoId: string, branchName: string, baseSha: string): Promise<void> => {
    const request = phgit.BranchRequest.create({ repoPath: repoId, branchName, baseSha });
    const payload = phgit.BranchRequest.encode(request).finish();
    await invoke('create_branch', payload);
};

export const createTag = async (repoId: string, tagName: string, sha: string): Promise<void> => {
    const request = phgit.TagRequest.create({ repoPath: repoId, tagName, sha });
    const payload = phgit.TagRequest.encode(request).finish();
    await invoke('create_tag', payload);
};

export const cherryPickCommit = async (repoId: string, sha: string): Promise<void> => {
    const request = phgit.CherryPickRequest.create({ repoPath: repoId, sha });
    const payload = phgit.CherryPickRequest.encode(request).finish();
    await invoke('cherry_pick', payload);
};

export const updateHook = async (repoId: string, hookType: keyof UserSettings['hooks'], content: string): Promise<void> => {
    const request = phgit.HookRequest.create({ repoPath: repoId, hookType, content });
    const payload = phgit.HookRequest.encode(request).finish();
    await invoke('update_hook', payload);
};
