import React from 'react';
import { Icon } from '../shared/Icon';
import { useAppContext } from '../../contexts/AppContext';

const ToolbarButton: React.FC<{ icon: string; label: string; onClick?: () => void }> = ({ icon, label, onClick }) => (
  <button onClick={onClick} className="flex items-center gap-2 px-3 py-1.5 text-sm bg-slate-700/50 hover:bg-slate-700 rounded-md transition-colors duration-150">
    <Icon name={icon} className="w-4 h-4" />
    <span>{label}</span>
  </button>
);

export const Toolbar: React.FC = () => {
  const { setActiveView, setIsSummaryModalOpen } = useAppContext();
  return (
    <header className="flex items-center justify-between p-2 bg-slate-800/50 border-b border-slate-700 flex-shrink-0">
      <div className="flex items-center gap-2">
        <ToolbarButton icon="arrow-down" label="Fetch" />
        <ToolbarButton icon="arrow-down-to-line" label="Pull" />
        <ToolbarButton icon="arrow-up-to-line" label="Push" />
        <ToolbarButton icon="git-branch" label="Branch" />
        <ToolbarButton icon="package" label="Stash" />
        <ToolbarButton icon="list-checks" label="Daily Summary" onClick={() => setIsSummaryModalOpen(true)} />
      </div>
      <div className="flex items-center gap-2">
         <button onClick={() => setActiveView('settings')} className="p-1.5 text-slate-400 hover:bg-slate-700 rounded-md transition-colors duration-150" aria-label="Open Settings">
            <Icon name="settings" className="w-5 h-5" />
         </button>
      </div>
    </header>
  );
};
