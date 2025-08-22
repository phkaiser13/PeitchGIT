import React, { useState } from 'react';
import { GitCommit } from 'lucide-react';

interface CommitInputProps {
    onCommit: (message: string) => void;
}

const CommitInput: React.FC<CommitInputProps> = ({ onCommit }) => {
    const [message, setMessage] = useState('');

    const handleSubmit = () => {
        if (message.trim()) {
            onCommit(message);
            setMessage('');
        }
    };

    return (
        <div className="flex flex-col space-y-2">
            <textarea
                value={message}
                onChange={(e) => setMessage(e.target.value)}
                placeholder="Commit message"
                className="w-full h-24 p-2 rounded bg-vscode-input-bg border border-vscode-border focus:outline-none focus:ring-1 focus:ring-blue-500"
            />
            <button
                onClick={handleSubmit}
                disabled={!message.trim()}
                className="bg-vscode-button-bg hover:bg-vscode-button-hover-bg disabled:opacity-50 text-white font-bold py-2 px-4 rounded flex items-center justify-center"
            >
                <GitCommit className="w-4 h-4 mr-2" />
                Commit
            </button>
        </div>
    );
};

export default CommitInput;
