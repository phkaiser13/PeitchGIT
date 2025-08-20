import React, { useState } from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { Icon } from '../shared/Icon';
import type { Account, GitService, GitServiceIdentifier } from '../../types';

const SERVICES: GitService[] = [
    { id: 'github', name: 'GitHub', icon: 'github', requiresUrl: false },
    { id: 'gitlab', name: 'GitLab.com', icon: 'gitlab', requiresUrl: false },
    { id: 'self_hosted_gitlab', name: 'GitLab Self-managed', icon: 'gitlab', requiresUrl: true },
    { id: 'gitea', name: 'Gitea', icon: 'git-branch', requiresUrl: true },
    { id: 'bitbucket', name: 'Bitbucket', icon: 'git-branch', requiresUrl: false },
    { id: 'ssh', name: 'SSH Server', icon: 'key-round', requiresUrl: true },
];

const ServiceButton: React.FC<{ service: GitService; onSelect: () => void; }> = ({ service, onSelect }) => (
    <button onClick={onSelect} className="flex flex-col items-center justify-center gap-2 p-4 bg-slate-700/50 hover:bg-slate-700 rounded-lg transition-colors w-full">
        <Icon name={service.icon} className="w-8 h-8" />
        <span className="text-sm font-semibold text-center">{service.name}</span>
    </button>
);


export const AddAccountModal: React.FC<{ onClose: () => void }> = ({ onClose }) => {
    const { setAccounts } = useAppContext();
    const [step, setStep] = useState(1);
    const [selectedService, setSelectedService] = useState<GitService | null>(null);

    const [instanceUrl, setInstanceUrl] = useState('');
    const [username, setUsername] = useState('');
    const [token, setToken] = useState('');
    const [isConnecting, setIsConnecting] = useState(false);
    const [error, setError] = useState('');

    const handleServiceSelect = (service: GitService) => {
        setSelectedService(service);
        setStep(2);
        if (!service.requiresUrl) {
            setInstanceUrl(`https://${service.id}.com`);
        }
    };

    const handleConnect = async () => {
        if (!selectedService || !username || !token) {
            setError("Please fill in all required fields.");
            return;
        }
        setIsConnecting(true);
        setError('');

        // Mock API call to verify credentials
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        // Mock success
        const newAccount: Account = {
            id: `${selectedService.id}-${username}-${Date.now()}`,
            provider: selectedService.id,
            username,
            instanceUrl: selectedService.requiresUrl ? instanceUrl : `https://${selectedService.id}.com`,
            token,
        };
        setAccounts(prev => [...prev, newAccount]);
        setIsConnecting(false);
        onClose();
    };
    
    const renderStep1 = () => (
        <>
            <h2 className="text-lg font-semibold text-white text-center mb-1">Connect a Git Account</h2>
            <p className="text-sm text-slate-400 text-center mb-6">Choose a service to connect to.</p>
            <div className="grid grid-cols-3 gap-4">
                {SERVICES.map(service => (
                    <ServiceButton key={service.id} service={service} onSelect={() => handleServiceSelect(service)} />
                ))}
            </div>
        </>
    );

    const renderStep2 = () => {
        if (!selectedService) return null;
        const isSsh = selectedService.id === 'ssh';
        return (
            <div>
                 <button onClick={() => setStep(1)} className="flex items-center gap-1 text-sm text-slate-400 hover:text-white mb-4">
                    <Icon name="arrow-left" className="w-4 h-4" /> Back to services
                 </button>
                 <div className="flex items-center gap-4 mb-6">
                    <Icon name={selectedService.icon} className="w-10 h-10" />
                    <div>
                        <h2 className="text-xl font-bold text-white">Connect to {selectedService.name}</h2>
                        <p className="text-slate-400">Enter your account details.</p>
                    </div>
                 </div>

                <div className="space-y-4">
                     {selectedService.requiresUrl && (
                        <div>
                            <label htmlFor="instance-url" className="block text-sm font-medium mb-1 text-slate-300">{isSsh ? 'Server Address' : 'Server URL'}</label>
                            <input 
                                type="text" 
                                id="instance-url" 
                                placeholder={isSsh ? 'git.example.com' : 'https://gitlab.mycompany.com'}
                                className="w-full bg-slate-900 border border-slate-600 rounded-md px-3 py-2" 
                                value={instanceUrl}
                                onChange={(e) => setInstanceUrl(e.target.value)}
                            />
                        </div>
                    )}
                     <div>
                        <label htmlFor="username" className="block text-sm font-medium mb-1 text-slate-300">Username</label>
                        <input 
                            type="text" 
                            id="username" 
                            placeholder="Your username"
                            className="w-full bg-slate-900 border border-slate-600 rounded-md px-3 py-2" 
                            value={username}
                            onChange={(e) => setUsername(e.target.value)}
                        />
                    </div>
                     <div>
                        <label htmlFor="token" className="block text-sm font-medium mb-1 text-slate-300">{isSsh ? 'SSH Key Path' : 'Personal Access Token'}</label>
                        <input 
                            type={isSsh ? 'text' : 'password'}
                            id="token" 
                            placeholder={isSsh ? '~/.ssh/id_ed25519' : 'Paste your token here'}
                            className="w-full bg-slate-900 border border-slate-600 rounded-md px-3 py-2 font-mono" 
                             value={token}
                            onChange={(e) => setToken(e.target.value)}
                        />
                        {!isSsh && <p className="text-xs text-slate-500 mt-1">Your token is stored securely and never shared.</p>}
                    </div>
                </div>
            </div>
        );
    }

    return (
        <div 
            className="fixed inset-0 bg-black/50 z-50 flex justify-center items-center" 
            onClick={onClose}
            aria-modal="true"
            role="dialog"
        >
            <div 
                className="w-full max-w-lg bg-slate-800 border border-slate-700 rounded-lg shadow-2xl flex flex-col"
                onClick={e => e.stopPropagation()}
            >
                <header className="flex items-center justify-end p-2 border-b border-slate-700">
                    <button onClick={onClose} className="p-1.5 text-slate-400 hover:text-white hover:bg-slate-700 rounded-md">
                        <Icon name="x" className="w-5 h-5" />
                    </button>
                </header>
                <main className="p-8">
                    {step === 1 && renderStep1()}
                    {step === 2 && renderStep2()}
                </main>
                {step === 2 && (
                    <footer className="flex justify-between items-center p-4 border-t border-slate-700">
                        {error && <p className="text-sm text-red-400">{error}</p>}
                        <div className="flex-grow"></div>
                         <button 
                            onClick={handleConnect}
                            disabled={isConnecting}
                            className="flex items-center justify-center gap-2 px-4 py-2 text-sm font-semibold bg-indigo-600 hover:bg-indigo-500 rounded-md transition-colors text-white disabled:bg-slate-600 w-32"
                         >
                            {isConnecting ? <Icon name="loader-2" className="w-4 h-4 animate-spin" /> : 'Connect'}
                        </button>
                    </footer>
                )}
            </div>
        </div>
    );
};
