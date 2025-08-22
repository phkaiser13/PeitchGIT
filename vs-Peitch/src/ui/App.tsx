import React, { useState, useEffect, useCallback } from 'react';
import { vscode } from './vscode';
import { GitStatus, Commit } from '../git/types';
import Overview from './Overview';
import CommitsStaging from './CommitsStaging';
import BranchView from './BranchView';
import { ExtensionToWebviewMessage } from '../common/types';

type Tab = 'Overview' | 'Commits & Staging' | 'Branch View';

const App: React.FC = () => {
    const [activeTab, setActiveTab] = useState<Tab>('Overview');
    const [status, setStatus] = useState<GitStatus | null>(null);
    const [commitLog, setCommitLog] = useState<Commit[]>([]);
    const [branchViewData, setBranchViewData] = useState<{ branch: string, files: string[] } | null>(null);
    const [error, setError] = useState<string | null>(null);

    const handleMessage = useCallback((event: MessageEvent<ExtensionToWebviewMessage>) => {
        const message = event.data;
        switch (message.type) {
            case 'status':
                setStatus(message.payload);
                setError(null);
                break;
            case 'commitLog':
                setCommitLog(message.payload);
                break;
            case 'branchFiles':
                setBranchViewData(message.payload);
                setActiveTab('Branch View');
                break;
            case 'showBranch': // Custom event from controller
                vscode.postMessage({ type: 'getFilesForBranch', payload: { branch: message.payload.branch } });
                break;
            case 'operationSuccess':
                 // Can show a toast notification here in the future
                console.log(message.payload.message);
                break;
            case 'error':
                setError(message.payload.message);
                break;
        }
    }, []);

    useEffect(() => {
        window.addEventListener('message', handleMessage);
        vscode.postMessage({ type: 'ready' }); // Inform the extension the webview is ready
        return () => {
            window.removeEventListener('message', handleMessage);
        };
    }, [handleMessage]);

    const renderTabContent = () => {
        switch (activeTab) {
            case 'Overview':
                return <Overview status={status} commitLog={commitLog} />;
            case 'Commits & Staging':
                return <CommitsStaging status={status} />;
            case 'Branch View':
                return <BranchView data={branchViewData} />;
            default:
                return null;
        }
    };
    
    const TABS: Tab[] = ['Overview', 'Commits & Staging', 'Branch View'];

    return (
        <main className="bg-vscode-bg text-vscode-text h-screen flex flex-col p-4 font-sans">
            <header className="border-b border-vscode-border pb-2 mb-4">
                <nav className="flex space-x-4">
                    {TABS.map(tab => (
                        <button 
                            key={tab}
                            onClick={() => setActiveTab(tab)}
                            className={`px-3 py-1 rounded ${activeTab === tab ? 'bg-vscode-list-active-bg' : 'hover:bg-vscode-list-hover-bg'}`}
                        >
                            {tab}
                        </button>
                    ))}
                </nav>
            </header>

            {error && (
                <div className="bg-red-500/20 border border-red-500 text-red-300 p-2 rounded mb-4">
                    <strong>Error:</strong> {error}
                </div>
            )}
            
            <div className="flex-grow overflow-y-auto">
                {renderTabContent()}
            </div>
        </main>
    );
};

export default App;
