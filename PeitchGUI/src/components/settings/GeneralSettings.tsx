import React from 'react';

const SettingRow: React.FC<{ label: string; description: string; children: React.ReactNode }> = ({ label, description, children }) => (
    <div className="grid grid-cols-1 md:grid-cols-3 gap-4 items-start py-4 border-b border-slate-700">
        <div className="md:col-span-1">
            <h4 className="font-semibold text-white">{label}</h4>
            <p className="text-sm text-slate-400">{description}</p>
        </div>
        <div className="md:col-span-2">
            {children}
        </div>
    </div>
);

export const GeneralSettings: React.FC = () => {
    return (
        <div>
            <h2 className="text-2xl font-bold text-white mb-1">General Settings</h2>
            <p className="text-slate-400 mb-6">Configure general application behavior.</p>
            
             <SettingRow label="Auto-fetch" description="Automatically fetch from all remotes periodically.">
                 <select className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2">
                    <option>Disabled</option>
                    <option>Every 5 minutes</option>
                    <option>Every 15 minutes</option>
                    <option>Every 30 minutes</option>
                </select>
            </SettingRow>

            <SettingRow label="Prune on fetch" description="Clean up stale remote-tracking branches during fetch.">
                <input type="checkbox" className="h-6 w-6 rounded bg-slate-700 border-slate-600 text-indigo-500 focus:ring-indigo-500" defaultChecked />
            </SettingRow>
        </div>
    );
};