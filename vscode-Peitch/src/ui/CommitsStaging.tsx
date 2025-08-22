import React, { useState, useEffect } from 'react';
import { GitStatus } from '../git/types';
import FileList from './components/FileList';
import CommitInput from './components/CommitInput';
import { vscode } from './vscode';

interface CommitsStagingProps {
    status: GitStatus | null;
}

const CommitsStaging: React.FC<CommitsStagingProps> = ({ status: initialStatus }) => {
    const [status, setStatus] = useState<GitStatus | null>(initialStatus);
    const [selectedUnstaged, setSelectedUnstaged] = useState<string[]>([]);
    const [selectedStaged, setSelectedStaged] = useState<string[]>([]);
    
    // Update local state when prop changes
    useEffect(() => {
        setStatus(initialStatus);
    }, [initialStatus]);

    // Request fresh status when component mounts
    useEffect(() => {
        vscode.postMessage({ type: 'getStatus' });
    }, []);

    const handleStage = (files: string[]) => {
        vscode.postMessage({ type: 'stageFiles', payload: { files } });
        setSelectedUnstaged(prev => prev.filter(f => !files.includes(f)));
    };

    const handleUnstage = (files: string[]) => {
        vscode.postMessage({ type: 'unstageFiles', payload: { files } });
        setSelectedStaged(prev => prev.filter(f => !files.includes(f)));
    };

    const handleCommit = (message: string) => {
        if (status?.staged && status.staged.length > 0) {
            vscode.postMessage({ type: 'commit', payload: { message } });
        } else {
            vscode.postMessage({ type: 'showError', payload: { message: "No files staged to commit." } });
        }
    };

    if (!status) {
        return <div>Loading changes...</div>;
    }

    const allUnstaged = [...status.unstaged, ...status.untracked];

    return (
        <div className="flex flex-col h-full">
            <div className="flex-grow space-y-4 overflow-y-auto">
                <FileList
                    title={`Staged Changes (${status.staged.length})`}
                    files={status.staged}
                    selectedFiles={selectedStaged}
                    onSelectionChange={setSelectedStaged}
                    onAction={handleUnstage}
                    actionIcon="unstage"
                />
                <FileList
                    title={`Changes (${allUnstaged.length})`}
                    files={allUnstaged}
                    selectedFiles={selectedUnstaged}
                    onSelectionChange={setSelectedUnstaged}
                    onAction={handleStage}
                    actionIcon="stage"
                />
            </div>
            <div className="mt-4 pt-4 border-t border-vscode-border">
                <CommitInput onCommit={handleCommit} />
            </div>
        </div>
    );
};

export default CommitsStaging;
