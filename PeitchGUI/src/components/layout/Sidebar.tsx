import React from 'react';
import type { Repository, Branch, Tag, Stash, Remote } from '../../types';
import { Icon } from '../shared/Icon';
import { useAppContext } from '../../contexts/AppContext';
import { useRepository } from '../../hooks/useRepository';

interface SidebarProps {
  repositories: Repository[];
  selectedRepo: Repository | null;
  branches: Branch[];
  tags: Tag[];
  stashes: Stash[];
  remotes: Remote[];
  onSelectRepo: (repoId: string) => void;
}

const SidebarSection: React.FC<{ title: string; children: React.ReactNode; count?: number }> = ({ title, children, count }) => (
  <div className="mb-4">
    <h3 className="px-3 text-xs font-bold uppercase text-slate-500 tracking-wider mb-2 flex justify-between items-center">
      <span>{title}</span>
      {typeof count !== 'undefined' && <span className="text-slate-400 font-mono text-[10px] bg-slate-700/50 px-1.5 py-0.5 rounded-full">{count}</span>}
    </h3>
    <ul>{children}</ul>
  </div>
);

const BranchItem: React.FC<{ branch: Branch, isCurrent: boolean, onSelect: () => void }> = ({ branch, isCurrent, onSelect }) => (
    <li onClick={onSelect} className={`flex items-center gap-2 px-3 py-1 text-sm truncate rounded-md cursor-pointer hover:bg-slate-700/50 ${isCurrent ? 'bg-blue-600/20 text-white font-semibold' : ''}`}>
        <Icon name={branch.isRemote ? 'cloud' : 'git-branch'} className="w-4 h-4 text-slate-400 flex-shrink-0" />
        <span className="truncate">{branch.name}</span>
  </li>
);

const TagItem: React.FC<{ tag: Tag }> = ({ tag }) => (
  <li className="flex items-center gap-2 px-3 py-1 text-sm truncate hover:bg-slate-700/50 rounded-md cursor-pointer">
    <Icon name="tag" className="w-4 h-4 text-slate-400 flex-shrink-0" />
    <span className="truncate">{tag.name}</span>
  </li>
);

const StashItem: React.FC<{ stash: Stash }> = ({ stash }) => (
  <li className="flex items-center gap-2 px-3 py-1 text-sm truncate hover:bg-slate-700/50 rounded-md cursor-pointer">
    <Icon name="package" className="w-4 h-4 text-slate-400 flex-shrink-0" />
    <span className="truncate">{stash.message}</span>
  </li>
);

const RemoteItem: React.FC<{ remote: Remote }> = ({ remote }) => (
  <li className="flex flex-col px-3 py-1 text-sm truncate hover:bg-slate-700/50 rounded-md cursor-pointer">
    <div className="flex items-center gap-2">
      <Icon name="server" className="w-4 h-4 text-slate-400 flex-shrink-0" />
      <span className="truncate font-semibold">{remote.name}</span>
    </div>
    <span className="text-xs text-slate-500 pl-6 truncate">{remote.url}</span>
  </li>
);


export const Sidebar: React.FC<SidebarProps> = ({ repositories, selectedRepo, branches, tags, stashes, remotes, onSelectRepo }) => {
  const { currentBranch } = useAppContext();
  const { checkoutBranch } = useRepository();

  return (
    <aside className="p-2">
      <SidebarSection title="Repositories" count={repositories.length}>
        {repositories.map(repo => (
          <li
            key={repo.id}
            onClick={() => onSelectRepo(repo.id)}
            className={`flex items-center gap-2 px-3 py-1.5 text-sm truncate rounded-md cursor-pointer ${selectedRepo?.id === repo.id ? 'bg-blue-600/30 text-white' : 'hover:bg-slate-700/50'}`}
          >
            <Icon name="git-fork" className="w-4 h-4 text-slate-400 flex-shrink-0" />
            <span className="font-semibold truncate">{repo.name}</span>
          </li>
        ))}
      </SidebarSection>
      <SidebarSection title="Branches" count={branches.filter(b => !b.isRemote).length}>
        {branches.filter(b => !b.isRemote).map(branch => 
          <BranchItem 
            key={branch.name} 
            branch={branch} 
            isCurrent={currentBranch?.name === branch.name}
            onSelect={() => checkoutBranch(branch)} 
          />
        )}
      </SidebarSection>
       <SidebarSection title="Stashes" count={stashes.length}>
        {stashes.map(stash => <StashItem key={stash.id} stash={stash} />)}
      </SidebarSection>
      <SidebarSection title="Remotes" count={remotes.length}>
         {remotes.map(remote => <RemoteItem key={remote.name} remote={remote} />)}
        {branches.filter(b => b.isRemote).map(branch => <BranchItem key={branch.name} branch={branch} isCurrent={false} onSelect={() => checkoutBranch(branch)} />)}
      </SidebarSection>
      <SidebarSection title="Tags" count={tags.length}>
        {tags.map(tag => <TagItem key={tag.name} tag={tag} />)}
      </SidebarSection>
    </aside>
  );
};