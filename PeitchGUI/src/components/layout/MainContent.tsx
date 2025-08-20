import React from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { CommitGraphView } from '../views/CommitGraphView';
import { WorkspaceView } from '../views/WorkspaceView';
import { SettingsView } from '../views/SettingsView';
import { Icon } from '../shared/Icon';

const TabButton: React.FC<{
  label: string;
  icon: string;
  isActive: boolean;
  onClick: () => void;
}> = ({ label, icon, isActive, onClick }) => (
  <button
    onClick={onClick}
    className={`flex items-center gap-2 px-4 py-2 text-sm font-medium border-b-2 transition-colors duration-150 ${
      isActive
        ? 'border-indigo-500 text-white'
        : 'border-transparent text-slate-400 hover:text-white'
    }`}
  >
    <Icon name={icon} className="w-4 h-4" />
    <span>{label}</span>
  </button>
);


export const MainContent: React.FC = () => {
    const { activeView, setActiveView } = useAppContext();

    const renderView = () => {
        switch (activeView) {
            case 'graph':
                return <CommitGraphView />;
            case 'workspace':
                return <WorkspaceView />;
            case 'settings':
                return <SettingsView />;
            default:
                return null;
        }
    }

    return (
        <div className="h-full flex flex-col bg-slate-900">
            <header className="flex-shrink-0 flex items-center border-b border-slate-700">
                <TabButton label="Graph" icon="git-merge" isActive={activeView === 'graph'} onClick={() => setActiveView('graph')} />
                <TabButton label="Workspace" icon="hard-drive" isActive={activeView === 'workspace'} onClick={() => setActiveView('workspace')} />
            </header>
            <main className="flex-grow overflow-hidden">
                {renderView()}
            </main>
        </div>
    );
};
