import React, { useState, useEffect } from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { generateDailySummary } from '../../services/geminiService';
import { Icon } from './Icon';
import type { Commit } from '../../types';

export const DailySummaryModal: React.FC = () => {
    const { setIsSummaryModalOpen, commits, settings } = useAppContext();
    const [summary, setSummary] = useState('');
    const [isLoading, setIsLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);
    const [copyButtonText, setCopyButtonText] = useState('Copy');

    useEffect(() => {
        const fetchSummary = async () => {
            setIsLoading(true);
            setError(null);

            const now = new Date();
            const twentyFourHoursAgo = new Date(now.getTime() - 24 * 60 * 60 * 1000);
            
            const recentCommits = commits.filter(c => {
                const commitDate = new Date(c.date);
                return c.author.email === settings.profile.email && commitDate > twentyFourHoursAgo;
            });

            if (recentCommits.length === 0) {
                setSummary("No commits found in the last 24 hours to summarize.");
                setIsLoading(false);
                return;
            }

            try {
                const result = await generateDailySummary(recentCommits, settings.profile.name);
                setSummary(result);
            } catch (err) {
                setError(err instanceof Error ? err.message : "An unknown error occurred.");
            } finally {
                setIsLoading(false);
            }
        };

        fetchSummary();
    }, [commits, settings.profile.email, settings.profile.name]);

    const handleCopy = () => {
        navigator.clipboard.writeText(summary);
        setCopyButtonText('Copied!');
        setTimeout(() => setCopyButtonText('Copy'), 2000);
    };

    return (
        <div 
            className="fixed inset-0 bg-black/50 z-50 flex justify-center items-center" 
            onClick={() => setIsSummaryModalOpen(false)}
            aria-modal="true"
            role="dialog"
        >
            <div 
                className="w-full max-w-2xl bg-slate-800 border border-slate-700 rounded-lg shadow-2xl flex flex-col max-h-[80vh]"
                onClick={e => e.stopPropagation()}
            >
                <header className="flex items-center justify-between p-4 border-b border-slate-700 flex-shrink-0">
                    <h2 className="text-lg font-semibold text-white">Your Daily Summary</h2>
                    <button onClick={() => setIsSummaryModalOpen(false)} className="text-slate-400 hover:text-white">
                        <Icon name="x" className="w-5 h-5" />
                    </button>
                </header>
                <main className="p-6 overflow-y-auto">
                    {isLoading && (
                        <div className="flex flex-col items-center justify-center text-slate-400 h-48">
                            <Icon name="loader-2" className="w-8 h-8 animate-spin mb-4" />
                            <p>Generating your summary with Gemini...</p>
                        </div>
                    )}
                    {error && <div className="text-red-400 bg-red-500/10 p-4 rounded-md">{error}</div>}
                    {!isLoading && !error && (
                        <div className="prose prose-invert prose-sm max-w-none" dangerouslySetInnerHTML={{ __html: summary.replace(/\n/g, '<br />') }}>
                        </div>
                    )}
                </main>
                <footer className="flex justify-end p-4 border-t border-slate-700 flex-shrink-0">
                    <button
                        onClick={handleCopy}
                        disabled={isLoading || !!error}
                        className="flex items-center gap-2 px-4 py-2 text-sm font-semibold bg-indigo-600 hover:bg-indigo-500 rounded-md text-white disabled:bg-slate-600"
                    >
                        <Icon name={copyButtonText === 'Copy' ? 'copy' : 'check'} className="w-4 h-4" />
                        {copyButtonText}
                    </button>
                </footer>
            </div>
        </div>
    );
};
