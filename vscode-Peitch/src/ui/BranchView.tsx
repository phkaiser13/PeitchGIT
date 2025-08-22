import React from 'react';
import { FileText } from 'lucide-react';

interface BranchViewProps {
    data: { branch: string; files: string[] } | null;
}

const BranchView: React.FC<BranchViewProps> = ({ data }) => {
    if (!data) {
        return <div className="text-vscode-text-secondary">Select a branch from the sidebar to view its files.</div>;
    }

    return (
        <div className="space-y-2">
            <h1 className="text-2xl font-semibold border-b border-vscode-border pb-2 mb-2">
                Files in <span className="font-mono bg-vscode-input-bg p-1 rounded">{data.branch}</span>
            </h1>
            <ul className="space-y-1">
                {data.files.map(file => (
                    <li key={file} className="flex items-center p-2 rounded hover:bg-vscode-list-hover-bg">
                        <FileText className="w-4 h-4 mr-3 text-vscode-text-secondary flex-shrink-0" />
                        <span className="truncate">{file}</span>
                    </li>
                ))}
            </ul>
        </div>
    );
};

export default BranchView;
