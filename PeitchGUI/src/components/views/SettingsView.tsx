import React from 'react';
import { Icon } from '../shared/Icon';
import { GeneralSettings } from '../settings/GeneralSettings';
import { ProfileSettings } from '../settings/ProfileSettings';
import { AppearanceSettings } from '../settings/AppearanceSettings';
import { AISettings } from '../settings/AISettings';
import { HooksSettings } from '../settings/HooksSettings';
import { AccountsSettings } from '../settings/AccountsSettings';
import { useAppContext } from '../../contexts/AppContext';
import type { SettingsTab } from '../../types';

const SettingsNavItem: React.FC<{
    icon: string;
    label: string;
    isActive: boolean;
    onClick: () => void;
}> = ({ icon, label, isActive, onClick }) => (
    <button
        onClick={onClick}
        className={`w-full flex items-center gap-3 px-3 py-2 text-left text-sm rounded-md transition-colors ${
            isActive ? 'bg-slate-700 text-white font-semibold' : 'text-slate-300 hover:bg-slate-700/50'
        }`}
    >
        <Icon name={icon} className="w-5 h-5" />
        <span>{label}</span>
    </button>
);


export const SettingsView: React.FC = () => {
    const { settingsTab, setSettingsTab } = useAppContext();

    const renderContent = () => {
        switch (settingsTab) {
            case 'profile':
                return <ProfileSettings />;
            case 'appearance':
                return <AppearanceSettings />;
            case 'general':
                 return <GeneralSettings />;
            case 'ai':
                return <AISettings />;
            case 'hooks':
                return <HooksSettings />;
            case 'accounts':
                return <AccountsSettings />;
            default:
                return null;
        }
    };

    return (
        <div className="h-full flex text-slate-300">
            <aside className="w-56 flex-shrink-0 bg-slate-800/50 p-4 border-r border-slate-700">
                <h1 className="text-lg font-bold text-white mb-6 px-3">Settings</h1>
                <nav className="space-y-1">
                    <SettingsNavItem icon="user-circle" label="Profile" isActive={settingsTab === 'profile'} onClick={() => setSettingsTab('profile')} />
                    <SettingsNavItem icon="at-sign" label="Accounts" isActive={settingsTab === 'accounts'} onClick={() => setSettingsTab('accounts')} />
                    <SettingsNavItem icon="paintbrush" label="Appearance" isActive={settingsTab === 'appearance'} onClick={() => setSettingsTab('appearance')} />
                    <SettingsNavItem icon="brain-circuit" label="AI" isActive={settingsTab === 'ai'} onClick={() => setSettingsTab('ai')} />
                    <SettingsNavItem icon="terminal-square" label="Hooks" isActive={settingsTab === 'hooks'} onClick={() => setSettingsTab('hooks')} />
                    <SettingsNavItem icon="settings-2" label="General" isActive={settingsTab === 'general'} onClick={() => setSettingsTab('general')} />
                </nav>
            </aside>
            <main className="flex-1 p-8 overflow-y-auto">
                <div className="max-w-3xl mx-auto">
                    {renderContent()}
                </div>
            </main>
        </div>
    );
};