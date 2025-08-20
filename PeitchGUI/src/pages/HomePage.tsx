import React from 'react';
import { AppLayout } from '../components/layout/AppLayout';
import { Toolbar } from '../components/layout/Toolbar';
import { Sidebar } from '../components/layout/Sidebar';
import { MainContent } from '../components/layout/MainContent';
import { CommitDetails } from '../components/details/CommitDetails';
import { StatusBar } from '../components/layout/StatusBar';
import { useAppContext } from '../contexts/AppContext';
import { useRepository } from '../hooks/useRepository';

export const HomePage: React.FC = () => {
  const { 
    repositories, 
    selectedRepo, 
    branches, 
    tags, 
    stashes,
    remotes,
    selectedCommit 
  } = useAppContext();
  
  const { selectRepo } = useRepository();

  return (
    <AppLayout
      toolbar={<Toolbar />}
      sidebar={
        <Sidebar
          repositories={repositories}
          selectedRepo={selectedRepo}
          branches={branches}
          tags={tags}
          stashes={stashes}
          remotes={remotes}
          onSelectRepo={selectRepo}
        />
      }
      mainContent={<MainContent />}
      detailsPanel={<CommitDetails commit={selectedCommit} />}
      statusBar={<StatusBar />}
    />
  );
};
