import { useCallback } from 'react';
import { 
  getRepositories, 
  getRepositoryById, 
  getWorkspaceStatus,
  stageFile as gitStageFile,
  unstageFile as gitUnstageFile,
  stageAll as gitStageAll,
  unstageAll as gitUnstageAll,
  createBranch as gitCreateBranch,
  createTag as gitCreateTag,
  cherryPickCommit as gitCherryPickCommit,
  checkoutBranch as gitCheckoutBranch,
  commit as gitCommit,
  updateHook as gitUpdateHook,
} from '../services/peitchGitService';
import { useAppContext } from '../contexts/AppContext';
import type { Commit, WorkspaceFileChange, Branch, Repository, UserSettings } from '../types';

export const useRepository = () => {
  const {
    selectedRepo,
    setRepositories,
    setSelectedRepo,
    setCommits,
    setBranches,
    setTags,
    setStashes,
    setRemotes,
    selectedCommit,
    setSelectedCommit,
    workspaceStatus,
    setWorkspaceStatus,
    setActiveView,
    setCurrentBranch,
    settings,
    currentBranch: currentBranchFromContext,
  } = useAppContext();

  const refreshRepoData = useCallback(async (repoId: string) => {
    const data = await getRepositoryById(repoId);
    const sortedCommits = data.commits.sort((a, b) => new Date(b.date).getTime() - new Date(a.date).getTime());
    
    setCommits(sortedCommits);
    setBranches(data.branches);
    setTags(data.tags);
    setStashes(data.stashes);
    setRemotes(data.remotes);
    
    if (sortedCommits.length > 0) {
      setSelectedCommit(current => current ? sortedCommits.find(c => c.sha === current.sha) || sortedCommits[0] : sortedCommits[0]);
    }
    
    const currentBranchName = currentBranchFromContext?.name;
    const updatedCurrentBranch = data.branches.find(b => b.name === currentBranchName) || data.branches.find(b => b.name === 'main' || b.name === 'master') || data.branches.find(b => !b.isRemote) || null;
    setCurrentBranch(updatedCurrentBranch);

  }, [setBranches, setCommits, setRemotes, setStashes, setTags, setSelectedCommit, setCurrentBranch, currentBranchFromContext]);

  const refreshWorkspace = useCallback(async () => {
    if (!selectedRepo) return;
    const workspace = await getWorkspaceStatus(selectedRepo.id);
    setWorkspaceStatus(workspace);
  }, [selectedRepo, setWorkspaceStatus]);


  const selectRepo = useCallback(async (repoId: string) => {
    const repos = await getRepositories();
    const repoMetadata = repos.find(r => r.id === repoId);
    if (!repoMetadata) return;

    const repoDetails = await getRepositoryById(repoId);

    const repoData: Repository = {
      ...repoMetadata,
      ...repoDetails,
    };
    
    setSelectedRepo(repoData);
    const sortedCommits = repoDetails.commits.sort((a, b) => new Date(b.date).getTime() - new Date(a.date).getTime());
    setCommits(sortedCommits);
    setBranches(repoDetails.branches);
    setTags(repoDetails.tags);
    setStashes(repoDetails.stashes);
    setRemotes(repoDetails.remotes);
    
    if (sortedCommits.length > 0) {
      setSelectedCommit(sortedCommits[0]);
    } else {
      setSelectedCommit(null);
    }

    const mainBranch = repoDetails.branches.find(b => b.name === 'main' || b.name === 'master') || repoDetails.branches.find(b => !b.isRemote) || null;
    setCurrentBranch(mainBranch);
    
    await refreshWorkspace();
  }, [refreshWorkspace, setBranches, setCommits, setCurrentBranch, setRemotes, setSelectedCommit, setSelectedRepo, setStashes, setTags]);

  const loadInitialData = useCallback(async () => {
    const repos = await getRepositories();
    setRepositories(repos as Repository[]);
    if (repos.length > 0) {
      await selectRepo(repos[0].id);
    }
  }, [setRepositories, selectRepo]);

  const selectCommit = useCallback((commit: Commit) => {
    setSelectedCommit(commit);
  }, [setSelectedCommit]);

  const checkoutBranch = useCallback(async (branch: Branch) => {
    if (!selectedRepo) return;
    if (branch.isRemote) {
      alert("Cannot checkout a remote branch directly.");
      return;
    }
    try {
        await gitCheckoutBranch(selectedRepo.id, branch.name);
        setCurrentBranch(branch);
        await refreshRepoData(selectedRepo.id);
        await refreshWorkspace();
        alert(`Switched to branch '${branch.name}'`);
    } catch(e: any) {
        alert(`Error checking out branch: ${e.message}`);
    }
  }, [selectedRepo, setCurrentBranch, refreshWorkspace, refreshRepoData]);

  const stageFile = useCallback(async (file: WorkspaceFileChange) => {
    if (!selectedRepo) return;
    await gitStageFile(selectedRepo.id, file.path);
    await refreshWorkspace();
  }, [selectedRepo, refreshWorkspace]);
  
  const unstageFile = useCallback(async (file: WorkspaceFileChange) => {
    if (!selectedRepo) return;
    await gitUnstageFile(selectedRepo.id, file.path);
    await refreshWorkspace();
  }, [selectedRepo, refreshWorkspace]);

  const stageAllFiles = useCallback(async () => {
    if (!selectedRepo) return;
    await gitStageAll(selectedRepo.id);
    await refreshWorkspace();
  }, [selectedRepo, refreshWorkspace]);

  const unstageAllFiles = useCallback(async () => {
    if (!selectedRepo) return;
    await gitUnstageAll(selectedRepo.id);
    await refreshWorkspace();
  }, [selectedRepo, refreshWorkspace]);
  
  const commit = useCallback(async (message: string) => {
    if (!selectedRepo || !workspaceStatus?.stagedFiles.length) {
      throw new Error("No staged files to commit.");
    }
    const { name, email } = settings.profile;
    await gitCommit(selectedRepo.id, message, name, email);
    await refreshRepoData(selectedRepo.id);
    await refreshWorkspace();
  }, [selectedRepo, workspaceStatus, settings.profile, refreshRepoData, refreshWorkspace]);

  // Staging/unstaging hunks would require backend support for applying partial patches.
  const stageHunk = useCallback((file: WorkspaceFileChange, hunkContent: string) => {
    console.warn("Staging hunks is not yet implemented in peitchGitService.");
  }, []);

  const unstageHunk = useCallback((file: WorkspaceFileChange, hunkContent: string) => {
    console.warn("Unstaging hunks is not yet implemented in peitchGitService.");
  }, []);

  const createBranch = useCallback(async (name: string, sha: string) => {
    if (!selectedRepo) return;
    await gitCreateBranch(selectedRepo.id, name, sha);
    await refreshRepoData(selectedRepo.id);
    alert(`Branch '${name}' created at ${sha.substring(0,7)}`);
  }, [selectedRepo, refreshRepoData]);

  const createTag = useCallback(async (name: string, sha: string) => {
    if (!selectedRepo) return;
    await gitCreateTag(selectedRepo.id, name, sha);
    await refreshRepoData(selectedRepo.id);
    alert(`Tag '${name}' created at ${sha.substring(0,7)}`);
  }, [selectedRepo, refreshRepoData]);

  const cherryPickCommit = useCallback(async (commit: Commit) => {
    if (!selectedRepo) return;
     try {
        await gitCherryPickCommit(selectedRepo.id, commit.sha);
        await refreshWorkspace();
        await refreshRepoData(selectedRepo.id);
        alert(`Changes from commit ${commit.shortSha} have been applied. Check the Workspace tab.`);
        setActiveView('workspace');
    } catch (e: any) {
        alert(`Cherry-pick failed: ${e.message}\n\nThis may be due to a conflict. Conflict resolution is not yet supported.`);
        await refreshWorkspace();
    }
  }, [selectedRepo, refreshWorkspace, setActiveView, refreshRepoData]);
  
  const updateHook = useCallback(async (hookType: keyof UserSettings['hooks'], content: string) => {
    if (!selectedRepo) {
        alert("No repository selected to update hook.");
        return;
    }
    try {
        await gitUpdateHook(selectedRepo.id, hookType, content);
    } catch(e: any) {
        alert(`Failed to update hook: ${e.message}`);
    }
  }, [selectedRepo]);

  return {
    loadInitialData,
    selectRepo,
    selectCommit,
    refreshWorkspace,
    stageFile,
    unstageFile,
    stageAllFiles,
    unstageAllFiles,
    stageHunk,
    unstageHunk,
    createBranch,
    createTag,
    cherryPickCommit,
    checkoutBranch,
    commit,
    updateHook,
  };
};
