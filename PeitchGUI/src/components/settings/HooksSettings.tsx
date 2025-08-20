import React, { useState } from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { useRepository } from '../../hooks/useRepository';
import { Icon } from '../shared/Icon';
import type { UserSettings } from '../../types';

const SettingRow: React.FC<{ label: string; description: string; children: React.ReactNode }> = ({ label, description, children }) => (
    <div className="grid grid-cols-1 md:grid-cols-3 gap-4 items-center py-4 border-b border-slate-700 last:border-b-0">
        <div className="md:col-span-1">
            <h4 className="font-semibold text-white">{label}</h4>
            <p className="text-sm text-slate-400">{description}</p>
        </div>
        <div className="md:col-span-2">
            {children}
        </div>
    </div>
);

type HookName = keyof UserSettings['hooks'];

export const HooksSettings: React.FC = () => {
    const { settings, setSettings } = useAppContext();
    const { updateHook } = useRepository();
    const [editingHook, setEditingHook] = useState<HookName | null>(null);
    const [scriptContent, setScriptContent] = useState('');

    const handleToggle = (hookName: HookName) => {
        setSettings(s => ({
            ...s,
            hooks: {
                ...s.hooks,
                [hookName]: { ...s.hooks[hookName], enabled: !s.hooks[hookName].enabled }
            }
        }));
    };

    const handleEditClick = (hookName: HookName) => {
        setScriptContent(settings.hooks[hookName].content);
        setEditingHook(hookName);
    };
    
    const handleSaveScript = () => {
        if (!editingHook) return;
        
        // Persist the change to the backend
        updateHook(editingHook, scriptContent);

        // Optimistically update the local state
        setSettings(s => ({
            ...s,
            hooks: {
                ...s.hooks,
                [editingHook]: { ...s.hooks[editingHook], content: scriptContent }
            }
        }));
        setEditingHook(null);
    };

    const renderHookEditorModal = () => {
        if (!editingHook) return null;
        return (
             <div 
                className="fixed inset-0 bg-black/50 z-50 flex justify-center items-center" 
                onClick={() => setEditingHook(null)}
                aria-modal="true"
                role="dialog"
            >
                <div 
                    className="w-full max-w-2xl bg-slate-800 border border-slate-700 rounded-lg shadow-2xl flex flex-col"
                    onClick={e => e.stopPropagation()}
                >
                    <header className="flex items-center justify-between p-4 border-b border-slate-700">
                        <h2 className="text-lg font-semibold text-white">Edit Hook: <span className="font-mono">{editingHook}</span></h2>
                        <button onClick={() => setEditingHook(null)} className="text-slate-400 hover:text-white">
                            <Icon name="x" className="w-5 h-5" />
                        </button>
                    </header>
                    <main className="p-4">
                        <textarea
                            value={scriptContent}
                            onChange={(e) => setScriptContent(e.target.value)}
                            className="w-full h-80 bg-slate-900 border border-slate-600 rounded-md p-2 font-mono text-sm text-slate-200 focus:ring-2 focus:ring-indigo-500"
                        />
                    </main>
                    <footer className="flex justify-end gap-3 p-4 border-t border-slate-700">
                         <button onClick={() => setEditingHook(null)} className="px-4 py-2 text-sm font-semibold bg-slate-600 hover:bg-slate-500 rounded-md text-white">
                            Cancel
                        </button>
                        <button onClick={handleSaveScript} className="px-4 py-2 text-sm font-semibold bg-indigo-600 hover:bg-indigo-500 rounded-md text-white">
                            Save Script
                        </button>
                    </footer>
                </div>
            </div>
        );
    }

    return (
        <div>
            <h2 className="text-2xl font-bold text-white mb-1">Git Hooks</h2>
            <p className="text-slate-400 mb-6">Automate your workflow by enabling and configuring Git hooks for this repository.</p>

            <div className="p-4 bg-slate-800 rounded-lg">
                {(Object.keys(settings.hooks) as HookName[]).map(hookName => {
                    const hook = settings.hooks[hookName];
                    const descriptions: Record<HookName, string> = {
                        'pre-commit': 'Runs before a commit is created. Good for linters or formatters.',
                        'pre-push': 'Runs before pushing code. Ideal for running tests.',
                        'commit-msg': 'Checks the commit message format before completing a commit.',
                    };

                    return (
                        <SettingRow key={hookName} label={hookName} description={descriptions[hookName]}>
                            <div className="flex items-center gap-4">
                                <label className="flex items-center cursor-pointer">
                                    <input type="checkbox" className="sr-only" checked={hook.enabled} onChange={() => handleToggle(hookName)} />
                                    <div className={`w-10 h-6 rounded-full transition-colors ${hook.enabled ? 'bg-indigo-500' : 'bg-slate-600'}`}>
                                        <div className={`w-4 h-4 bg-white rounded-full transition-transform m-1 ${hook.enabled ? 'translate-x-4' : ''}`}></div>
                                    </div>
                                    <span className="ml-3 text-sm font-medium">{hook.enabled ? 'Enabled' : 'Disabled'}</span>
                                </label>
                                <button onClick={() => handleEditClick(hookName)} className="text-sm font-semibold text-indigo-400 hover:text-indigo-300">
                                    Edit Script
                                </button>
                            </div>
                        </SettingRow>
                    )
                })}
            </div>
            {renderHookEditorModal()}
        </div>
    );
};
