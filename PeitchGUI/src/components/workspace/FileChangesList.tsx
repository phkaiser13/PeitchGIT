import React from 'react';
import type { WorkspaceFileChange } from '../../types';
import { Icon } from '../shared/Icon';

interface FileChangesListProps {
    title: string;
    files: WorkspaceFileChange[];
    selectedFile: WorkspaceFileChange | null;
    onFileSelect: (file: WorkspaceFileChange) => void;
    action: 'stage' | 'unstage';
    onStage?: (file: WorkspaceFileChange) => void;
    onUnstage?: (file: WorkspaceFileChange) => void;
    onStageAll?: () => void;
    onUnstageAll?: () => void;
}

const ActionButton: React.FC<{ icon: string; onClick: () => void, 'aria-label': string }> = ({ icon, onClick, ...props }) => (
    <button onClick={onClick} className="p-0.5 text-slate-400 hover:text-white hover:bg-slate-600 rounded" {...props}>
        <Icon name={icon} className="w-4 h-4" />
    </button>
);


export const FileChangesList: React.FC<FileChangesListProps> = ({
    title,
    files,
    selectedFile,
    onFileSelect,
    action,
    onStage,
    onUnstage,
    onStageAll,
    onUnstageAll
}) => {
    const handleAction = (file: WorkspaceFileChange) => {
        if (action === 'stage' && onStage) onStage(file);
        if (action === 'unstage' && onUnstage) onUnstage(file);
    }
    
    const handleAllAction = () => {
        if (action === 'stage' && onStageAll) onStageAll();
        if (action === 'unstage' && onUnstageAll) onUnstageAll();
    }

    const getStatusClass = (status: WorkspaceFileChange['status']) => {
        switch (status) {
            case 'added': return 'text-green-400';
            case 'deleted': return 'text-red-400';
            case 'modified': return 'text-yellow-400';
            default: return 'text-slate-400';
        }
    }

    return (
        <div>
            <div className="flex justify-between items-center mb-2">
                <h3 className="text-sm font-semibold text-slate-300">{title} ({files.length})</h3>
                {files.length > 0 && (
                    <button 
                        onClick={handleAllAction}
                        className="text-xs font-semibold text-slate-300 hover:text-white px-2 py-1 rounded-md hover:bg-slate-700"
                    >
                        {action === 'stage' ? 'Stage All' : 'Unstage All'}
                    </button>
                )}
            </div>
            {files.length > 0 ? (
                <ul className="space-y-1 border border-slate-700 rounded-md p-1">
                    {files.map(file => (
                        <li
                            key={file.id}
                            onClick={() => onFileSelect(file)}
                            className={`group flex items-center justify-between p-2 cursor-pointer rounded-md ${selectedFile?.id === file.id ? 'bg-blue-600/20' : 'hover:bg-slate-700/50'}`}
                        >
                            <div className="flex items-center gap-2 truncate">
                                <span className={`font-bold text-xs w-2 text-center ${getStatusClass(file.status)}`}>
                                    {file.status.charAt(0).toUpperCase()}
                                </span>
                                <span className="font-mono text-sm truncate">{file.path}</span>
                            </div>
                            <div className="opacity-0 group-hover:opacity-100 transition-opacity">
                                <ActionButton
                                    icon={action === 'stage' ? 'plus' : 'minus'}
                                    onClick={() => handleAction(file)}
                                    aria-label={action === 'stage' ? 'Stage file' : 'Unstage file'}
                                />
                            </div>
                        </li>
                    ))}
                </ul>
            ) : (
                <div className="text-center text-sm text-slate-500 py-4 border border-dashed border-slate-700 rounded-md">
                    No {title.toLowerCase()}
                </div>
            )}
        </div>
    );
};
