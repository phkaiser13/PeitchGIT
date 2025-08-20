import React from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { Icon } from '../shared/Icon';


const SettingInput: React.FC<{label: string; id: string; value: string; onChange: (e: React.ChangeEvent<HTMLInputElement>) => void}> = ({label, id, value, onChange}) => (
    <div>
        <label htmlFor={id} className="block text-sm font-medium mb-1 text-slate-300">{label}</label>
        <input 
            type="text" 
            id={id} 
            className="w-full bg-slate-700 border border-slate-600 rounded-md px-3 py-2 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500" 
            value={value}
            onChange={onChange}
        />
    </div>
)


export const ProfileSettings: React.FC = () => {
    const { settings, setSettings } = useAppContext();
    
    const handleProfileChange = (field: 'name' | 'email') => (e: React.ChangeEvent<HTMLInputElement>) => {
        setSettings(s => ({
            ...s,
            profile: {
                ...s.profile,
                [field]: e.target.value
            }
        }));
    };

    return (
        <div>
            <h2 className="text-2xl font-bold text-white mb-1">Profile</h2>
            <p className="text-slate-400 mb-6">This information will be used for your commits.</p>

            <div className="p-6 bg-slate-800 rounded-lg space-y-4">
                <div className="flex items-center gap-4">
                    <img
                        src={`https://i.pravatar.cc/64?u=${settings.profile.email}`}
                        alt={settings.profile.name}
                        className="w-16 h-16 rounded-full"
                    />
                    <div className="flex-1 space-y-2">
                        <SettingInput label="Author Name" id="author-name" value={settings.profile.name} onChange={handleProfileChange('name')} />
                        <SettingInput label="Author Email" id="author-email" value={settings.profile.email} onChange={handleProfileChange('email')} />
                    </div>
                </div>
                 <button className="w-full flex items-center justify-center gap-2 mt-4 px-4 py-2 text-sm font-semibold bg-indigo-600 hover:bg-indigo-500 rounded-md transition-colors duration-150 text-white">
                    <Icon name="save" className="w-4 h-4" />
                    Save Profile
                </button>
            </div>
        </div>
    );
};
