import React, { useEffect } from 'react';
import { GitStatus, Commit } from '../git/types';
import { vscode } from './vscode';
import { GitCommit, GitPullRequest, GitMerge, RefreshCw, ArrowUp, ArrowDown } from 'lucide-react';

interface OverviewProps {
    status: GitStatus | null;
    commitLog: Commit[];
}

const Overview: React.FC<OverviewProps> = ({ status, commitLog }) => {

    useEffect(() => {
        // Request fresh data when the tab becomes visible
        vscode.postMessage({ type: 'getStatus' });
        vscode.postMessage({ type: 'getCommitLog' });
    }, []);

    const handleFetch = () => vscode.postMessage({ type: 'fetch' });
    const handlePush = () => vscode.postMessage({ type: 'push' });
    const handleRefresh = () => vscode.postMessage({ type: 'getStatus' });

    if (!status) {
        return <div>Loading repository status...</div>;
    }

    return (
        <div className="space-y-6">
            <section className="flex justify-between items-center">
                <div>
                    <h1 className="text-2xl font-semibold">{status.branch}</h1>
                    {status.ahead !== undefined && status.behind !== undefined && (
                         <div className="flex items-center space-x-4 text-vscode-text-secondary">
                            <span><ArrowUp className="inline-block w-4 h-4 mr-1"/> {status.ahead} ahead</span>
                            <span><ArrowDown className="inline-block w-4 h-4 mr-1"/> {status.behind} behind</span>
                        </div>
                    )}
                </div>
                <div className="flex space-x-2">
                    <button onClick={handleFetch} className="bg-vscode-button-bg hover:bg-vscode-button-hover-bg px-3 py-2 rounded flex items-center"><GitPullRequest className="w-4 h-4 mr-2" /> Fetch</button>
                    <button onClick={handlePush} className="bg-vscode-button-bg hover:bg-vscode-button-hover-bg px-3 py-2 rounded flex items-center"><GitMerge className="w-4 h-4 mr-2" /> Push</button>
                    <button onClick={handleRefresh} className="bg-vscode-button-bg hover:bg-vscode-button-hover-bg px-3 py-2 rounded"><RefreshCw className="w-4 h-4" /></button>
                </div>
            </section>
            
            <section>
                <h2 className="text-xl font-semibold mb-2 border-b border-vscode-border pb-1">Recent Commits</h2>
                <ul className="space-y-2">
                    {commitLog.map(commit => (
                        <li key={commit.sha} className="flex items-center p-2 rounded hover:bg-vscode-list-hover-bg">
                            <GitCommit className="w-4 h-4 mr-3 text-vscode-text-secondary" />
                            <div>
                                <p className="font-mono text-xs text-vscode-text-secondary">{commit.sha.substring(0, 7)}</p>
                                <p>{commit.message}</p>
                            </div>
                        </li>
                    ))}
                </ul>
            </section>
        </div>
    );
};

export default Overview;
