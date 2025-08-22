import React from 'react';
import { Plus, Minus, Check, Circle } from 'lucide-react';

interface FileListProps {
    title: string;
    files: string[];
    selectedFiles: string[];
    onSelectionChange: (files: string[]) => void;
    onAction: (files: string[]) => void;
    actionIcon: 'stage' | 'unstage';
}

const FileList: React.FC<FileListProps> = ({ title, files, selectedFiles, onSelectionChange, onAction, actionIcon }) => {
    
    const toggleSelection = (file: string) => {
        if (selectedFiles.includes(file)) {
            onSelectionChange(selectedFiles.filter(f => f !== file));
        } else {
            onSelectionChange([...selectedFiles, file]);
        }
    };

    const handleHeaderAction = () => {
        if (files.length > 0) {
            onAction(files);
            onSelectionChange([]);
        }
    };
    
    const getFileColor = (file: string): string => {
        // This is a simplified version. A real implementation would parse git status more deeply.
        if (file.startsWith(' D ')) return 'text-git-deleted';
        if (file.startsWith(' M ')) return 'text-git-modified';
        if (file.startsWith('A ')) return 'text-git-added';
        if (file.startsWith('??')) return 'text-git-untracked';
        return 'text-vscode-text';
    };

    const ActionIcon = actionIcon === 'stage' ? Plus : Minus;

    return (
        <section>
            <header className="flex justify-between items-center mb-2 p-2 bg-vscode-input-bg rounded-t">
                <h3 className="font-semibold">{title}</h3>
                <button 
                    onClick={handleHeaderAction} 
                    title={actionIcon === 'stage' ? 'Stage All' : 'Unstage All'}
                    className="p-1 hover:bg-vscode-list-hover-bg rounded"
                >
                    <ActionIcon className="w-4 h-4" />
                </button>
            </header>
            <ul className="space-y-1">
                {files.map(file => {
                    const isSelected = selectedFiles.includes(file);
                    return (
                        <li 
                            key={file} 
                            className={`flex items-center p-2 rounded cursor-pointer ${isSelected ? 'bg-vscode-list-active-bg' : 'hover:bg-vscode-list-hover-bg'}`}
                            onClick={() => toggleSelection(file)}
                        >
                            {isSelected ? <Check className="w-4 h-4 mr-2" /> : <Circle className="w-4 h-4 mr-2 text-transparent group-hover:text-vscode-text-secondary" />}
                            <span className={`flex-grow truncate ${getFileColor(file)}`}>{file.substring(3)}</span>
                            <button 
                                onClick={(e) => { e.stopPropagation(); onAction([file]); }}
                                title={actionIcon === 'stage' ? 'Stage file' : 'Unstage file'}
                                className="ml-2 p-1 hover:bg-vscode-bg rounded opacity-50 hover:opacity-100"
                            >
                                <ActionIcon className="w-4 h-4" />
                            </button>
                        </li>
                    );
                })}
                {files.length === 0 && <li className="text-vscode-text-secondary text-sm px-2">No files in this category.</li>}
            </ul>
        </section>
    );
};

export default FileList;
