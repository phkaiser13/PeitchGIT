import React from 'react';
import { useAppContext } from '../../contexts/AppContext';
import type { AIProvider } from '../../types';

const SettingRow: React.FC<{ label: string; description: string; children: React.ReactNode; align?: 'center' | 'start' }> = ({ label, description, children, align = 'center' }) => (
    <div className={`grid grid-cols-1 md:grid-cols-3 gap-4 items-${align} py-4 border-b border-slate-700`}>
        <div className="md:col-span-1">
            <h4 className="font-semibold text-white">{label}</h4>
            <p className="text-sm text-slate-400">{description}</p>
        </div>
        <div className="md:col-span-2">
            {children}
        </div>
    </div>
);

const MODELS = {
    gemini: ['gemini-2.5-flash'],
    openai: ['gpt-4', 'gpt-4-32k', 'gpt-3.5-turbo'],
    anthropic: ['claude-2', 'claude-instant-1'],
};

export const AISettings: React.FC = () => {
    const { settings, setSettings } = useAppContext();

    const handleProviderChange = <K extends keyof (typeof settings.ai.providers[AIProvider])>(
        provider: AIProvider, 
        field: K, 
        value: (typeof settings.ai.providers[AIProvider])[K]
    ) => {
        setSettings(s => ({
            ...s,
            ai: {
                ...s.ai,
                providers: {
                    ...s.ai.providers,
                    [provider]: {
                        ...s.ai.providers[provider],
                        [field]: value
                    }
                }
            }
        }));
    };

    return (
        <div>
            <h2 className="text-2xl font-bold text-white mb-1">AI Settings</h2>
            <p className="text-slate-400 mb-6">Configure AI-powered features like commit message generation.</p>

            <SettingRow label="Enable AI Features" description="Allow PeitchGUI to use AI services to assist you.">
                <input 
                    type="checkbox" 
                    className="h-6 w-6 rounded bg-slate-700 border-slate-600 text-indigo-500 focus:ring-indigo-500" 
                    checked={settings.ai.enabled}
                    onChange={(e) => setSettings(s => ({...s, ai: {...s.ai, enabled: e.target.checked}}))}
                />
            </SettingRow>

            <SettingRow label="Default Provider" description="Select the default AI service to use for generating content.">
                 <select 
                    className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2"
                    value={settings.ai.defaultProvider}
                    onChange={(e) => setSettings(s => ({...s, ai: {...s.ai, defaultProvider: e.target.value as AIProvider}}))}
                    disabled={!settings.ai.enabled}
                >
                    <option value="gemini">Google Gemini</option>
                    <option value="openai">OpenAI</option>
                    <option value="anthropic">Anthropic</option>
                </select>
            </SettingRow>

            <div className="mt-8">
                <h3 className="text-xl font-bold text-white mb-4">Provider Configuration</h3>
                
                {/* Gemini Settings */}
                <div className="p-4 bg-slate-800/50 rounded-lg mb-4">
                    <h4 className="font-semibold text-lg text-white mb-2">Google Gemini</h4>
                     <SettingRow label="Model" description="Select the Gemini model to use." align="start">
                        <select 
                            className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2"
                            value={settings.ai.providers.gemini.selectedModel}
                            onChange={(e) => handleProviderChange('gemini', 'selectedModel', e.target.value)}
                            disabled={!settings.ai.enabled}
                        >
                            {MODELS.gemini.map(m => <option key={m} value={m}>{m}</option>)}
                        </select>
                     </SettingRow>
                     <SettingRow label="API Key" description="Your Gemini API key is managed securely." align="start">
                        <p className="text-sm text-slate-400 italic mt-2">The Gemini API key is configured via secure environment variables and is not editable here.</p>
                     </SettingRow>
                </div>

                {/* OpenAI Settings */}
                 <div className="p-4 bg-slate-800/50 rounded-lg mb-4">
                    <h4 className="font-semibold text-lg text-white mb-2">OpenAI</h4>
                     <SettingRow label="Model" description="Select the OpenAI model to use." align="start">
                        <select 
                            className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2"
                             value={settings.ai.providers.openai.selectedModel}
                             onChange={(e) => handleProviderChange('openai', 'selectedModel', e.target.value)}
                            disabled={!settings.ai.enabled}
                        >
                             {MODELS.openai.map(m => <option key={m} value={m}>{m}</option>)}
                        </select>
                     </SettingRow>
                     <SettingRow label="API Key" description="Enter your OpenAI API key." align="start">
                         <input 
                            type="password"
                            placeholder="sk-..."
                            className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2 font-mono"
                            value={settings.ai.providers.openai.apiKey}
                            onChange={(e) => handleProviderChange('openai', 'apiKey', e.target.value)}
                            disabled={!settings.ai.enabled}
                        />
                     </SettingRow>
                </div>

                {/* Anthropic Settings */}
                 <div className="p-4 bg-slate-800/50 rounded-lg">
                    <h4 className="font-semibold text-lg text-white mb-2">Anthropic</h4>
                     <SettingRow label="Model" description="Select the Anthropic model to use." align="start">
                        <select 
                            className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2"
                            value={settings.ai.providers.anthropic.selectedModel}
                            onChange={(e) => handleProviderChange('anthropic', 'selectedModel', e.target.value)}
                            disabled={!settings.ai.enabled}
                        >
                             {MODELS.anthropic.map(m => <option key={m} value={m}>{m}</option>)}
                        </select>
                     </SettingRow>
                     <SettingRow label="API Key" description="Enter your Anthropic API key." align="start">
                        <input 
                            type="password"
                            placeholder="Your Anthropic Key..."
                            className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2 font-mono"
                            value={settings.ai.providers.anthropic.apiKey}
                            onChange={(e) => handleProviderChange('anthropic', 'apiKey', e.target.value)}
                            disabled={!settings.ai.enabled}
                        />
                     </SettingRow>
                </div>

            </div>

        </div>
    );
};