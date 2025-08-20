import React, { useMemo } from 'react';
import { useAppContext } from '../contexts/AppContext';
import { useRepository } from './useRepository';
import type { Command, SettingsTab } from '../types';

export const useCommands = (): Command[] => {
    const { 
        setActiveView, 
        setIsCommandPaletteOpen,
        branches,
        repositories,
        setSettingsTab,
     } = useAppContext();
    const { 
        stageAllFiles, 
        unstageAllFiles,
        checkoutBranch,
        selectRepo,
        createBranch
    } = useRepository();

    const staticCommands = useMemo<Command[]>(() => [
        // Navigation
        {
            id: 'view.graph',
            name: 'Go to Graph View',
            keywords: 'log history commit',
            icon: 'git-merge',
            section: 'Navigation',
            onExecute: () => setActiveView('graph'),
        },
        {
            id: 'view.workspace',
            name: 'Go to Workspace View',
            keywords: 'files staging stage',
            icon: 'hard-drive',
            section: 'Navigation',
            onExecute: () => setActiveView('workspace'),
        },
        {
            id: 'view.settings',
            name: 'Go to Settings',
            icon: 'settings',
            section: 'Navigation',
            onExecute: () => setActiveView('settings'),
        },
        // Settings sub-navigation
        ...((['profile', 'appearance', 'ai', 'general', 'accounts'] as SettingsTab[]).map(tab => ({
             id: `settings.goto.${tab}`,
             name: `Settings: Go to ${tab.charAt(0).toUpperCase() + tab.slice(1)}`,
             keywords: `settings options config ${tab}`,
             icon: 'settings-2',
             section: 'Settings' as const,
             onExecute: () => {
                 setActiveView('settings');
                 setSettingsTab(tab);
             },
        }))),
        // Git Actions
        {
            id: 'git.stageAll',
            name: 'Stage All Changes',
            icon: 'plus-square',
            section: 'Git',
            onExecute: stageAllFiles,
        },
        {
            id: 'git.unstageAll',
            name: 'Unstage All Changes',
            icon: 'minus-square',
            section: 'Git',
            onExecute: unstageAllFiles,
        },
        {
            id: 'git.createBranch',
            name: 'Create new branch...',
            icon: 'git-branch-plus',
            section: 'Git',
            onExecute: () => {
                const branchName = prompt('Enter new branch name:');
                if (branchName) {
                    // In a real app, we'd select a base commit. Here we use HEAD.
                    const headSha = branches.find(b => b.isRemote === false)?.sha;
                    if(headSha) createBranch(branchName, headSha);
                    else alert("Could not determine HEAD commit.");
                }
            },
        },

    ], [setActiveView, setSettingsTab, stageAllFiles, unstageAllFiles, branches, createBranch]);

    const branchCommands = useMemo<Command[]>(() =>
        branches
            .filter(b => !b.isRemote)
            .map(branch => ({
                id: `git.checkout.${branch.name}`,
                name: `Checkout branch '${branch.name}'`,
                keywords: branch.name,
                icon: 'git-branch',
                section: 'Git',
                onExecute: () => checkoutBranch(branch),
            })
    ), [branches, checkoutBranch]);
    
    const repoCommands = useMemo<Command[]>(() => 
        repositories.map(repo => ({
            id: `repo.select.${repo.id}`,
            name: `Switch to repository '${repo.name}'`,
            keywords: repo.name,
            icon: 'git-fork',
            section: 'Repository',
            onExecute: () => selectRepo(repo.id),
        })
    ), [repositories, selectRepo]);

    return useMemo(() => {
        const allCommands = [
            ...staticCommands,
            ...branchCommands,
            ...repoCommands,
        ];
        // Wrap all onExecute calls to close the palette automatically
        return allCommands.map(cmd => ({
            ...cmd,
            onExecute: () => {
                cmd.onExecute();
                setIsCommandPaletteOpen(false);
            }
        }));
    }, [staticCommands, branchCommands, repoCommands, setIsCommandPaletteOpen]);
};