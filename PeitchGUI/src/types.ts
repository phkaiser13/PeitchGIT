export interface FileChange {
  path: string;
  status: 'added' | 'deleted' | 'modified' | 'renamed';
  diff: string;
}

export interface WorkspaceFileChange extends FileChange {
    id: string;
    staged: boolean;
}

export interface WorkspaceStatus {
    stagedFiles: WorkspaceFileChange[];
    unstagedFiles: WorkspaceFileChange[];
}

export interface Commit {
  sha: string;
  shortSha: string;
  message: string;
  author: {
    name: string;
    email: string;
  };
  date: string;
  parents: string[];
  files: FileChange[];
  branch: string; // This is a simplification; a commit can be in multiple branches
}

export interface Branch {
  name: string;
  isRemote: boolean;
  sha: string;
}

export interface Tag {
  name: string;
  sha: string;
}

export interface Stash {
    id: string;
    message: string;
    date: string;
}

export interface Remote {
    name: string;
    url: string;
}

export interface Repository {
  id: string;
  name:string;
  path: string;
  commits: Commit[];
  branches: Branch[];
  tags: Tag[];
  stashes: Stash[];
  remotes: Remote[];
}

export type AIProvider = 'gemini' | 'openai' | 'anthropic';

export interface AIProviderConfig {
  apiKey: string;
  selectedModel: string;
}

export interface AIConfig {
  enabled: boolean;
  defaultProvider: AIProvider;
  providers: {
    gemini: AIProviderConfig;
    openai: AIProviderConfig;
    anthropic: AIProviderConfig;
  };
}

export interface GitHook {
    enabled: boolean;
    content: string;
}

export interface UserSettings {
    profile: {
        name: string;
        email: string;
    },
    appearance: {
        theme: 'dark' | 'light';
        fontSize: number;
    },
    ai: AIConfig;
    hooks: {
        'pre-commit': GitHook;
        'pre-push': GitHook;
        'commit-msg': GitHook;
    }
}

export type SettingsTab = 'profile' | 'appearance' | 'accounts' | 'general' | 'ai' | 'hooks';

export interface Command {
    id: string;
    name: string;
    keywords?: string;
    icon: string;
    shortcut?: {
        display: string;
        keys: string[];
    };
    onExecute: () => void;
    section: 'Navigation' | 'Git' | 'Repository' | 'Settings';
}

export interface SecretScanIssue {
    path: string;
    reason: string;
}

export interface SecretScanResult {
    found: boolean;
    issues: SecretScanIssue[];
}

// --- New Types for Git Service Accounts ---

export type GitServiceIdentifier = 
    | 'github' 
    | 'gitlab' 
    | 'bitbucket' 
    | 'azure_devops' 
    | 'gitea' 
    | 'aws_codecommit' 
    | 'self_hosted_gitlab'
    | 'ssh';

export interface GitService {
    id: GitServiceIdentifier;
    name: string;
    icon: string;
    requiresUrl: boolean;
}

export interface Account {
    id: string; // e.g., 'github-my-username'
    provider: GitServiceIdentifier;
    username: string;
    instanceUrl: string; // e.g., 'https://github.com' or 'https://gitlab.mycompany.com'
    token: string; // This would be encrypted in a real app
}
