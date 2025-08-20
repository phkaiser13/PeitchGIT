import React, { useState, useEffect } from 'react';
import { useAppContext } from '../../contexts/AppContext';
import type { WorkspaceFileChange } from '../../types';
import { useRepository } from '../../hooks/useRepository';
import { FileChangesList } from '../workspace/FileChangesList';
import { CommitForm } from '../workspace/CommitForm';
import { FileDiffView } from '../details/FileDiffView';

export const WorkspaceView: React.FC = () => {
    const { workspaceStatus } = useAppContext();
    const { stageFile, unstageFile, stageAllFiles, unstageAllFiles, stageHunk, unstageHunk } = useRepository();
    const [selectedFile, setSelectedFile] = useState<WorkspaceFileChange | null>(null);

    const stagedFiles = workspaceStatus?.stagedFiles || [];
    const unstagedFiles = workspaceStatus?.unstagedFiles || [];

    useEffect(() => {
        // If current selection is gone, clear it
        if (selectedFile) {
            const allFiles = [...unstagedFiles, ...stagedFiles];
            if (!allFiles.find(f => f.id === selectedFile.id)) {
                setSelectedFile(null);
            }
        }
    }, [workspaceStatus, selectedFile, unstagedFiles, stagedFiles]);


    useEffect(() => {
        if (!selectedFile && (unstagedFiles.length > 0 || stagedFiles.length > 0)) {
            setSelectedFile(unstagedFiles[0] || stagedFiles[0]);
        }
    }, [workspaceStatus, selectedFile, unstagedFiles, stagedFiles]);

    if (!workspaceStatus) {
        return <div className="p-4 text-slate-500">Loading workspace...</div>;
    }

     const handleStageHunk = (hunkContent: string) => {
        if (selectedFile) {
            stageHunk(selectedFile, hunkContent);
        }
    };

    const handleUnstageHunk = (hunkContent: string) => {
        if (selectedFile) {
            unstageHunk(selectedFile, hunkContent);
        }
    };
    
    return (
        <div className="h-full grid grid-cols-2 overflow-hidden">
            <div className="col-span-1 flex flex-col overflow-hidden border-r border-slate-700">
                <div className="flex-1 overflow-y-auto p-4">
                    <FileChangesList
                        title="Unstaged Files"
                        files={unstagedFiles}
                        onFileSelect={setSelectedFile}
                        selectedFile={selectedFile}
                        onStage={stageFile}
                        onStageAll={stageAllFiles}
                        action="stage"
                    />
                    <div className="mt-4">
                        <FileChangesList
                            title="Staged Files"
                            files={stagedFiles}
                            onFileSelect={setSelectedFile}
                            selectedFile={selectedFile}
                            onUnstage={unstageFile}
                            onUnstageAll={unstageAllFiles}
                            action="unstage"
                        />
                    </div>
                </div>
                <div className="flex-shrink-0 p-4 border-t border-slate-700 bg-slate-800">
                    <CommitForm stagedFiles={stagedFiles} />
                </div>
            </div>
            <div className="col-span-1 overflow-y-auto">
                {selectedFile ? (
                    <FileDiffView
                        file={selectedFile}
                        onStageHunk={handleStageHunk}
                        onUnstageHunk={handleUnstageHunk}
                    />
                ) : (
                    <div className="h-full flex items-center justify-center text-slate-500 bg-slate-900">
                        <div className="text-center">
                            <h3 className="font-semibold">No file selected</h3>
                            <p>Select a file to view its changes.</p>
                        </div>
                    </div>
                )}
            </div>
        </div>
    );
};