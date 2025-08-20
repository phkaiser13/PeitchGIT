import React, { createContext, useState, useContext, useMemo } from 'react';
import type { Repository, Commit, Branch, Tag, Stash, Remote, WorkspaceStatus, UserSettings, SettingsTab, Account } from '../types';

const DEFAULT_SETTINGS: UserSettings = {
    profile: {
        name: 'Alex Doe',
        email: 'alex@example.com'
    },
    appearance: {
        theme: 'dark',
        fontSize: 14,
    },
    ai: {
        enabled: true, // Enabled by default now
        defaultProvider: 'gemini',
        providers: {
            gemini: { apiKey: '', selectedModel: 'gemini-2.5-flash' },
            openai: { apiKey: '', selectedModel: 'gpt-3.5-turbo' },
            anthropic: { apiKey: '', selectedModel: 'claude-instant-1' },
        }
    },
    hooks: {
        'pre-commit': { enabled: true, content: '#!/bin/sh\n# Run linters\nnpm run lint' },
        'pre-push': { enabled: false, content: '#!/bin/sh\n# Run tests before pushing\nnpm test' },
        'commit-msg': { enabled: false, content: '#!/bin/sh\n# Check commit message format\n# Example: check against Conventional Commits spec' },
    }
}

interface AppState {
  repositories: Repository[];
  selectedRepo: Repository | null;
  commits: Commit[];
  branches: Branch[];
  tags: Tag[];
  stashes: Stash[];
  remotes: Remote[];
  selectedCommit: Commit | null;
  workspaceStatus: WorkspaceStatus | null;
  settings: UserSettings;
  activeView: 'graph' | 'workspace' | 'settings';
  currentBranch: Branch | null;
  isCommandPaletteOpen: boolean;
  isSummaryModalOpen: boolean;
  settingsTab: SettingsTab;
  accounts: Account[];
}

interface AppContextType extends AppState {
  setRepositories: React.Dispatch<React.SetStateAction<Repository[]>>;
  setSelectedRepo: React.Dispatch<React.SetStateAction<Repository | null>>;
  setCommits: React.Dispatch<React.SetStateAction<Commit[]>>;
  setBranches: React.Dispatch<React.SetStateAction<Branch[]>>;
  setTags: React.Dispatch<React.SetStateAction<Tag[]>>;
  setStashes: React.Dispatch<React.SetStateAction<Stash[]>>;
  setRemotes: React.Dispatch<React.SetStateAction<Remote[]>>;
  setSelectedCommit: React.Dispatch<React.SetStateAction<Commit | null>>;
  setWorkspaceStatus: React.Dispatch<React.SetStateAction<WorkspaceStatus | null>>;
  setSettings: React.Dispatch<React.SetStateAction<UserSettings>>;
  setActiveView: React.Dispatch<React.SetStateAction<'graph' | 'workspace' | 'settings'>>;
  setCurrentBranch: React.Dispatch<React.SetStateAction<Branch | null>>;
  setIsCommandPaletteOpen: React.Dispatch<React.SetStateAction<boolean>>;
  setIsSummaryModalOpen: React.Dispatch<React.SetStateAction<boolean>>;
  setSettingsTab: React.Dispatch<React.SetStateAction<SettingsTab>>;
  setAccounts: React.Dispatch<React.SetStateAction<Account[]>>;
}

const AppContext = createContext<AppContextType | undefined>(undefined);

export const AppProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [repositories, setRepositories] = useState<Repository[]>([]);
  const [selectedRepo, setSelectedRepo] = useState<Repository | null>(null);
  const [commits, setCommits] = useState<Commit[]>([]);
  const [branches, setBranches] = useState<Branch[]>([]);
  const [tags, setTags] = useState<Tag[]>([]);
  const [stashes, setStashes] = useState<Stash[]>([]);
  const [remotes, setRemotes] = useState<Remote[]>([]);
  const [selectedCommit, setSelectedCommit] = useState<Commit | null>(null);
  const [workspaceStatus, setWorkspaceStatus] = useState<WorkspaceStatus | null>(null);
  const [settings, setSettings] = useState<UserSettings>(DEFAULT_SETTINGS);
  const [activeView, setActiveView] = useState<'graph' | 'workspace' | 'settings'>('graph');
  const [currentBranch, setCurrentBranch] = useState<Branch | null>(null);
  const [isCommandPaletteOpen, setIsCommandPaletteOpen] = useState<boolean>(false);
  const [isSummaryModalOpen, setIsSummaryModalOpen] = useState<boolean>(false);
  const [settingsTab, setSettingsTab] = useState<SettingsTab>('profile');
  const [accounts, setAccounts] = useState<Account[]>([]);


  const value = useMemo(() => ({
    repositories, setRepositories,
    selectedRepo, setSelectedRepo,
    commits, setCommits,
    branches, setBranches,
    tags, setTags,
    stashes, setStashes,
    remotes, setRemotes,
    selectedCommit, setSelectedCommit,
    workspaceStatus, setWorkspaceStatus,
    settings, setSettings,
    activeView, setActiveView,
    currentBranch, setCurrentBranch,
    isCommandPaletteOpen, setIsCommandPaletteOpen,
    isSummaryModalOpen, setIsSummaryModalOpen,
    settingsTab, setSettingsTab,
    accounts, setAccounts,
  }), [repositories, selectedRepo, commits, branches, tags, stashes, remotes, selectedCommit, workspaceStatus, settings, activeView, currentBranch, isCommandPaletteOpen, isSummaryModalOpen, settingsTab, accounts]);

  return <AppContext.Provider value={value}>{children}</AppContext.Provider>;
};

export const useAppContext = () => {
  const context = useContext(AppContext);
  if (context === undefined) {
    throw new Error('useAppContext must be used within an AppProvider');
  }
  return context;
};