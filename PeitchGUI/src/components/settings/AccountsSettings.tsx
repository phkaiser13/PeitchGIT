import React, { useState } from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { Icon } from '../shared/Icon';
import { AddAccountModal } from './AddAccountModal';
import type { Account } from '../../types';

const SERVICE_INFO: Record<Account['provider'], { name: string; icon: string }> = {
    github: { name: 'GitHub', icon: 'github' },
    gitlab: { name: 'GitLab', icon: 'gitlab' },
    bitbucket: { name: 'Bitbucket', icon: 'git-branch' }, // No direct icon
    gitea: { name: 'Gitea', icon: 'git-branch' },
    azure_devops: { name: 'Azure DevOps', icon: 'server' },
    aws_codecommit: { name: 'AWS CodeCommit', icon: 'server' },
    self_hosted_gitlab: { name: 'GitLab (Self-hosted)', icon: 'gitlab' },
    ssh: { name: 'SSH', icon: 'key-round' },
};

export const AccountsSettings: React.FC = () => {
    const { accounts, setAccounts } = useAppContext();
    const [isModalOpen, setIsModalOpen] = useState(false);

    const removeAccount = (accountId: string) => {
        if (window.confirm('Are you sure you want to remove this account?')) {
            setAccounts(prev => prev.filter(acc => acc.id !== accountId));
        }
    };

    return (
        <div>
            <div className="flex justify-between items-center mb-6">
                <div>
                    <h2 className="text-2xl font-bold text-white mb-1">Accounts</h2>
                    <p className="text-slate-400">Manage connections to your Git hosting services.</p>
                </div>
                <button 
                    onClick={() => setIsModalOpen(true)}
                    className="flex items-center gap-2 px-4 py-2 text-sm font-semibold bg-indigo-600 hover:bg-indigo-500 rounded-md transition-colors text-white"
                >
                    <Icon name="plus" className="w-4 h-4" />
                    Add Account
                </button>
            </div>
            
            <div className="bg-slate-800 rounded-lg p-4 space-y-3">
                {accounts.length > 0 ? (
                    accounts.map(account => {
                        const service = SERVICE_INFO[account.provider];
                        return (
                            <div key={account.id} className="flex items-center justify-between p-3 bg-slate-700/50 rounded-md">
                                <div className="flex items-center gap-4">
                                    <Icon name={service.icon} className="w-6 h-6 text-slate-300" />
                                    <div>
                                        <p className="font-semibold text-white">{account.username}</p>
                                        <p className="text-sm text-slate-400">{service.name} - <span className="font-mono text-xs">{account.instanceUrl}</span></p>
                                    </div>
                                </div>
                                <button 
                                    onClick={() => removeAccount(account.id)}
                                    className="p-1.5 text-slate-400 hover:text-red-400 hover:bg-red-500/10 rounded-md"
                                >
                                    <Icon name="trash-2" className="w-4 h-4" />
                                </button>
                            </div>
                        )
                    })
                ) : (
                    <div className="text-center py-8 text-slate-500">
                        <Icon name="at-sign" className="w-10 h-10 mx-auto mb-2" />
                        <p>No accounts connected.</p>
                        <p className="text-sm">Add an account to get started.</p>
                    </div>
                )}
            </div>

            {isModalOpen && <AddAccountModal onClose={() => setIsModalOpen(false)} />}
        </div>
    );
};