import React, { useState, useEffect } from 'react';
import type { Commit, FileChange } from '../../types';
import { FileDiffView } from './FileDiffView';

interface CommitDetailsProps {
  commit: Commit | null;
}

export const CommitDetails: React.FC<CommitDetailsProps> = ({ commit }) => {
  const [selectedFile, setSelectedFile] = useState<FileChange | null>(null);

  useEffect(() => {
    if (commit && commit.files.length > 0) {
      setSelectedFile(commit.files[0]);
    } else {
      setSelectedFile(null);
    }
  }, [commit]);
  

  if (!commit) {
    return (
      <div className="flex flex-col items-center justify-center h-full text-slate-500 p-4">
        <i data-lucide="info" className="w-12 h-12 mb-4"></i>
        <h3 className="font-semibold">No Commit Selected</h3>
        <p className="text-center">Select a commit from the graph to see its details and file changes.</p>
      </div>
    );
  }

  const handleFileSelect = (file: FileChange) => {
    setSelectedFile(file);
  };

  return (
    <div className="flex flex-col h-full bg-slate-800/50">
      <div className="p-4 border-b border-slate-700 flex-shrink-0">
        <h2 className="text-lg font-semibold text-white">{commit.message}</h2>
        <div className="flex items-center gap-2 mt-2 text-sm">
          <img
            src={`https://i.pravatar.cc/32?u=${commit.author.email}`}
            alt={commit.author.name}
            className="w-6 h-6 rounded-full"
          />
          <div>
            <span className="font-semibold text-slate-300">{commit.author.name}</span>
            <span className="text-slate-400"> committed on {new Date(commit.date).toDateString()}</span>
          </div>
        </div>
        <div className="mt-2 text-sm text-slate-400 font-mono">
          <span>Commit </span>
          <span className="text-amber-400">{commit.sha}</span>
        </div>
         <div className="mt-2 text-sm text-slate-400 font-mono">
          <span>Parents </span>
          {commit.parents.map(p => <span key={p} className="text-sky-400 ml-2">{p.substring(0,7)}</span>)}
        </div>
      </div>
      
      <div className="flex-grow p-4 overflow-y-auto">
        <h3 className="text-sm font-semibold mb-2 text-slate-300">
            {commit.files.length} changed file{commit.files.length !== 1 && 's'}
        </h3>
        {commit.files.length > 0 ? (
          <>
            <ul className="mb-4 border border-slate-700 rounded-md">
              {commit.files.map((file, index) => (
                <li
                  key={file.path}
                  onClick={() => handleFileSelect(file)}
                  className={`flex items-center justify-between p-2 cursor-pointer ${selectedFile?.path === file.path ? 'bg-blue-600/20' : 'hover:bg-slate-700/50'} ${index < commit.files.length - 1 ? 'border-b border-slate-700' : ''}`}
                >
                  <span className="font-mono text-sm">{file.path}</span>
                  <span className={`text-xs font-bold px-2 py-0.5 rounded-full ${
                    file.status === 'added' ? 'bg-green-500/20 text-green-400' :
                    file.status === 'deleted' ? 'bg-red-500/20 text-red-400' :
                    'bg-yellow-500/20 text-yellow-400'
                  }`}>{file.status.toUpperCase()}</span>
                </li>
              ))}
            </ul>
            {selectedFile && <FileDiffView file={selectedFile} />}
          </>
        ) : (
             <div className="text-center text-slate-500 py-4">This commit has no file changes.</div>
        )}
      </div>
    </div>
  );
};
