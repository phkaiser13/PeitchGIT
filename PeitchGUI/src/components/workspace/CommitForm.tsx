import React, { useState, useRef, useEffect } from 'react';
import { scanForSecrets, generateCommitMessage } from '../../services/geminiService';
import { Icon } from '../shared/Icon';
import type { WorkspaceFileChange, AIProvider, SecretScanResult } from '../../types';
import { useAppContext } from '../../contexts/AppContext';
import { useRepository } from '../../hooks/useRepository';

interface CommitFormProps {
  stagedFiles: WorkspaceFileChange[];
}

type ScanStatus = 'idle' | 'scanning' | 'success' | 'warning';

export const CommitForm: React.FC<CommitFormProps> = ({ stagedFiles }) => {
  const { settings } = useAppContext();
  const { commit } = useRepository();
  const [commitMessage, setCommitMessage] = useState('');
  const [isGenerating, setIsGenerating] = useState(false);
  const [isCommitting, setIsCommitting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [isModelSelectorOpen, setIsModelSelectorOpen] = useState(false);
  const dropdownRef = useRef<HTMLDivElement>(null);

  const [scanStatus, setScanStatus] = useState<ScanStatus>('idle');
  const [scanResult, setScanResult] = useState<SecretScanResult | null>(null);

  useEffect(() => {
    // Reset scan status when staged files change
    setScanStatus('idle');
    setScanResult(null);
    setError(null);
  }, [stagedFiles]);

  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
        setIsModelSelectorOpen(false);
      }
    };
    document.addEventListener('mousedown', handleClickOutside);
    return () => {
      document.removeEventListener('mousedown', handleClickOutside);
    };
  }, []);
  
  const handleCommit = async () => {
      if (!commitMessage.trim() || stagedFiles.length === 0) return;
      setIsCommitting(true);
      setError(null);
      try {
        await commit(commitMessage);
        setCommitMessage('');
      } catch (err) {
        setError(err instanceof Error ? err.message : 'An unknown error occurred during commit.');
      } finally {
        setIsCommitting(false);
      }
  };

  const handleGenerateMessage = async (provider?: AIProvider, model?: string) => {
    if (stagedFiles.length === 0) return;
    setIsGenerating(true);
    setError(null);
    setIsModelSelectorOpen(false);

    const useProvider = provider || settings.ai.defaultProvider;
    const useModel = model || settings.ai.providers[useProvider].selectedModel;
    const apiKey = settings.ai.providers[useProvider].apiKey;

    const diffSummary = stagedFiles
      .map(file => `--- a/${file.path}\n+++ b/${file.path}\n${file.diff}`)
      .join('\n\n');

    try {
      const message = await generateCommitMessage(diffSummary, useProvider, useModel, apiKey);
      setCommitMessage(message);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'An unknown error occurred.');
    } finally {
      setIsGenerating(false);
    }
  };
  
  const handleScanForSecrets = async () => {
    if (stagedFiles.length === 0) return;
    setScanStatus('scanning');
    setScanResult(null);
    setError(null);
    try {
      const diffs = stagedFiles.map(f => ({ path: f.path, diff: f.diff }));
      const result = await scanForSecrets(diffs);
      setScanResult(result);
      setScanStatus(result.found ? 'warning' : 'success');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'An unknown error occurred during scan.');
      setScanStatus('idle');
    }
  };

  const aiEnabled = settings.ai.enabled;

  const renderAiButton = () => {
    if (!aiEnabled) return null;
    const isDisabled = isGenerating || isCommitting || stagedFiles.length === 0;
    return (
       <div className="relative inline-flex" ref={dropdownRef}>
            <button
                onClick={() => handleGenerateMessage()}
                disabled={isDisabled}
                className="flex items-center gap-2 pl-3 pr-2 py-1 text-xs font-semibold bg-indigo-600 hover:bg-indigo-700 rounded-l-md disabled:bg-slate-600 disabled:cursor-not-allowed transition-colors duration-150 text-white"
                aria-label="Generate commit message with AI"
            >
                {isGenerating ? (
                    <Icon name="loader-2" className="w-4 h-4 animate-spin" />
                ) : (
                    <Icon name="sparkles" className="w-4 h-4" />
                )}
                <span>Generate</span>
            </button>
            <button 
                onClick={() => setIsModelSelectorOpen(prev => !prev)}
                disabled={isDisabled}
                className="px-1 py-1 bg-indigo-600 hover:bg-indigo-700 rounded-r-md disabled:bg-slate-600 transition-colors border-l border-indigo-500/50"
                 aria-label="Select AI model"
            >
                <Icon name="chevron-down" className="w-4 h-4" />
            </button>

            {isModelSelectorOpen && (
                 <div className="absolute bottom-full right-0 mb-2 w-64 bg-slate-700 border border-slate-600 rounded-md shadow-lg z-10">
                    {/* ... dropdown content ... */}
                </div>
            )}
       </div>
    );
  }
  
  const renderScanStatus = () => {
      if (stagedFiles.length === 0) return null;
      
      switch (scanStatus) {
          case 'scanning':
              return <div className="flex items-center gap-2 text-sm text-slate-400"><Icon name="loader-2" className="w-4 h-4 animate-spin" /><span>Scanning for secrets...</span></div>
          case 'success':
              return <div className="flex items-center gap-2 text-sm text-green-400"><Icon name="shield-check" className="w-4 h-4" /><span>No secrets found.</span></div>
          case 'warning':
              return <div className="flex items-center gap-2 text-sm text-yellow-400"><Icon name="shield-alert" className="w-4 h-4" /><span>Warning: {scanResult?.issues.length} potential secret(s) found.</span></div>
          case 'idle':
          default:
            return (
                <button onClick={handleScanForSecrets} className="flex items-center gap-2 text-sm text-slate-400 hover:text-white transition-colors">
                    <Icon name="shield" className="w-4 h-4" />
                    <span>Scan for Secrets</span>
                </button>
            )
      }
  }

  const isCommitButtonDisabled = !commitMessage.trim() || stagedFiles.length === 0 || isCommitting || isGenerating;

  return (
    <div>
      <div className="flex justify-between items-center mb-2">
        <label htmlFor="commit-message" className="text-sm font-semibold text-slate-300">Commit Message</label>
        {renderAiButton()}
      </div>
      <textarea
        id="commit-message"
        rows={4}
        className="w-full bg-slate-900 border border-slate-700 rounded-md px-3 py-2 text-white font-mono placeholder-slate-500 focus:ring-2 focus:ring-indigo-500"
        placeholder="feat: Describe your changes..."
        value={commitMessage}
        onChange={(e) => setCommitMessage(e.target.value)}
        disabled={isGenerating || isCommitting}
      />
      {error && <p className="text-sm text-red-400 mt-2">{error}</p>}
      <div className="flex items-center justify-between mt-3">
        <div className="min-h-[20px]">
            {renderScanStatus()}
        </div>
         <button
            onClick={handleCommit}
            className={`px-4 py-2 text-sm font-semibold rounded-md text-white disabled:bg-slate-600 disabled:cursor-not-allowed transition-colors flex items-center gap-2 ${scanStatus === 'warning' ? 'bg-amber-600 hover:bg-amber-500' : 'bg-green-600 hover:bg-green-500'}`}
            disabled={isCommitButtonDisabled}
        >
            {isCommitting && <Icon name="loader-2" className="w-4 h-4 animate-spin" />}
            Commit {stagedFiles.length} file(s)
        </button>
      </div>
    </div>
  );
};
