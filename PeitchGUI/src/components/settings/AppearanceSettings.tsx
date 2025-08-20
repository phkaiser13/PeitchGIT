import React from 'react';
import { useAppContext } from '../../contexts/AppContext';

const SettingRow: React.FC<{ label: string; description: string; children: React.ReactNode }> = ({ label, description, children }) => (
    <div className="grid grid-cols-1 md:grid-cols-3 gap-4 items-center py-4 border-b border-slate-700">
        <div className="md:col-span-1">
            <h4 className="font-semibold text-white">{label}</h4>
            <p className="text-sm text-slate-400">{description}</p>
        </div>
        <div className="md:col-span-2">
            {children}
        </div>
    </div>
);

export const AppearanceSettings: React.FC = () => {
    const { settings, setSettings } = useAppContext();

    return (
        <div>
            <h2 className="text-2xl font-bold text-white mb-1">Appearance</h2>
            <p className="text-slate-400 mb-6">Customize how PeitchGUI looks and feels.</p>

            <SettingRow label="Theme" description="Select your preferred color theme.">
                 <select 
                    id="theme" 
                    className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2"
                    value={settings.appearance.theme}
                    onChange={(e) => setSettings(s => ({...s, appearance: {...s.appearance, theme: e.target.value as 'dark' | 'light'}}))}
                >
                    <option value="dark">Dark (Default)</option>
                    <option value="light" disabled>Light (Coming soon)</option>
                </select>
            </SettingRow>

            <SettingRow label="Font Size" description="Adjust the main font size for the UI.">
                <input 
                    type="number" 
                    id="font-size" 
                    className="w-full max-w-xs bg-slate-700 border border-slate-600 rounded-md px-3 py-2" 
                    value={settings.appearance.fontSize}
                    onChange={(e) => setSettings(s => ({...s, appearance: {...s.appearance, fontSize: parseInt(e.target.value, 10)}}))}
                />
            </SettingRow>
        </div>
    );
};
