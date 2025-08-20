import React from 'react';
import type { FileChange, WorkspaceFileChange } from '../../types';
import { parseDiff, type Hunk } from '../../utils/diffUtils';

interface FileDiffViewProps {
  file: FileChange | WorkspaceFileChange;
  onStageHunk?: (hunkContent: string) => void;
  onUnstageHunk?: (hunkContent: string) => void;
}

const DiffHunk: React.FC<{
    hunk: Hunk;
    actionButton?: React.ReactNode;
}> = ({ hunk, actionButton }) => {
    const allLines = hunk.content.split('\n');
    const header = allLines.shift() || '';

    return (
        <div className="mb-2 rounded-md overflow-hidden border border-slate-700">
            <div className="flex justify-between items-center p-2 bg-slate-800/80 text-blue-300 text-xs">
                <span className="font-mono">{header}</span>
                {actionButton}
            </div>
            <pre className="text-xs">
                <code>
                    {allLines.map((line, index) => {
                        let bgColor = 'bg-slate-900';
                        let lineContent = line;
                        
                        if (line.startsWith('+')) {
                          bgColor = 'bg-green-500/10';
                          lineContent = line.substring(1);
                        } else if (line.startsWith('-')) {
                          bgColor = 'bg-red-500/10';
                          lineContent = line.substring(1);
                        }

                        return (
                          <div key={index} className={`flex ${bgColor}`}>
                            <span className="w-6 text-center text-slate-600 select-none sticky left-0">{line.startsWith('+') ? '+' : line.startsWith('-') ? '-' : ' '}</span>
                            <span className="flex-1 whitespace-pre-wrap break-all pl-2">{lineContent}</span>
                          </div>
                        );
                    })}
                </code>
            </pre>
        </div>
    );
};


export const FileDiffView: React.FC<FileDiffViewProps> = ({ file, onStageHunk, onUnstageHunk }) => {
  const isWorkspaceFile = 'staged' in file;
  const hunks = parseDiff(file.diff);

  if (!file.diff) {
     return (
        <div className="bg-slate-900 font-mono text-xs h-full">
            <div className="sticky top-0 bg-slate-800/80 backdrop-blur-sm p-3 font-bold text-slate-300 border-b border-slate-700">{file.path}</div>
            <div className="p-4 text-slate-500">No changes to display.</div>
        </div>
     )
  }

  // If not a workspace file or no handlers, show old non-interactive view for commit history
  if (!isWorkspaceFile || (!onStageHunk && !onUnstageHunk)) {
      const diffLines = file.diff.split('\n');
      return (
        <div className="bg-slate-900 font-mono text-xs overflow-x-auto h-full">
          <div className="sticky top-0 bg-slate-800/80 backdrop-blur-sm p-3 font-bold text-slate-300 border-b border-slate-700">{file.path}</div>
          <pre className="p-3">
            <code>
              {diffLines.map((line, index) => {
                let bgColor = '';
                let lineContent = line;
                
                if (line.startsWith('+')) {
                  bgColor = 'bg-green-500/10';
                  lineContent = line.substring(1);
                } else if (line.startsWith('-')) {
                  bgColor = 'bg-red-500/10';
                  lineContent = line.substring(1);
                } else if (line.startsWith('@@')) {
                  bgColor = 'bg-blue-500/10 text-blue-300';
                }

                return (
                  <div key={index} className={`flex ${bgColor}`}>
                    <span className="w-10 text-right pr-4 text-slate-600 select-none sticky left-0">{index + 1}</span>
                    <span className="w-6 text-center text-slate-600 select-none sticky left-10">{line.startsWith('+') ? '+' : line.startsWith('-') ? '-' : ' '}</span>
                    <span className="flex-1 whitespace-pre-wrap break-all">{lineContent}</span>
                  </div>
                );
              })}
            </code>
          </pre>
        </div>
      );
  }

  return (
    <div className="bg-slate-900 h-full overflow-y-auto">
      <div className="sticky top-0 bg-slate-800/80 backdrop-blur-sm p-3 font-bold text-slate-300 border-b border-slate-700">{file.path}</div>
      <div className="p-2">
        {hunks.map((hunk, index) => {
            const isStaged = (file as WorkspaceFileChange).staged;
            let actionButton: React.ReactNode = null;
            if (!isStaged && onStageHunk) {
                actionButton = (
                    <button 
                        onClick={() => onStageHunk(hunk.content)} 
                        className="text-xs font-semibold text-slate-300 hover:text-white px-2 py-1 rounded-md hover:bg-slate-700"
                        aria-label="Stage Hunk"
                    >
                        Stage Hunk
                    </button>
                );
            }
            if (isStaged && onUnstageHunk) {
                 actionButton = (
                    <button 
                        onClick={() => onUnstageHunk(hunk.content)} 
                        className="text-xs font-semibold text-slate-300 hover:text-white px-2 py-1 rounded-md hover:bg-slate-700"
                        aria-label="Unstage Hunk"
                    >
                        Unstage Hunk
                    </button>
                );
            }

            return <DiffHunk key={index} hunk={hunk} actionButton={actionButton} />;
        })}
      </div>
    </div>
  );
};