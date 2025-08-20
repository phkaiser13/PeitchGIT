import React from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { Icon } from '../shared/Icon';

export const StatusBar: React.FC = () => {
    const { selectedRepo, currentBranch } = useAppContext();

    return (
        <footer className="flex items-center justify-between px-4 py-1 bg-slate-800 border-t border-slate-700 text-sm text-slate-400 flex-shrink-0">
            <div className="flex items-center gap-4">
                 <div className="flex items-center gap-2">
                    <Icon name="git-branch" className="w-4 h-4" />
                    <span>{currentBranch?.name || '...'}</span>
                 </div>
                 <div className="flex items-center gap-2">
                    <Icon name="folder" className="w-4 h-4" />
                    <span>{selectedRepo?.path || '...'}</span>
                 </div>
            </div>
            <div className="flex items-center gap-2">
                <span>PeitchGUI</span>
            </div>
        </footer>
    );
};