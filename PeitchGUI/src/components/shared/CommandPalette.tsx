import React, { useState, useEffect, useRef, useMemo } from 'react';
import { useCommands } from '../../hooks/useCommands';
import { fuzzySearch } from '../../utils/fuzzySearch';
import type { Command } from '../../types';
import { Icon } from './Icon';
import { useAppContext } from '../../contexts/AppContext';

export const CommandPalette: React.FC = () => {
  const { setIsCommandPaletteOpen } = useAppContext();
  const allCommands = useCommands();
  const [query, setQuery] = useState('');
  const [selectedIndex, setSelectedIndex] = useState(0);
  const inputRef = useRef<HTMLInputElement>(null);
  const listRef = useRef<HTMLUListElement>(null);

  const filteredCommands = useMemo(() => {
    return allCommands.filter(cmd => 
        fuzzySearch(query, cmd.name) || fuzzySearch(query, cmd.keywords || '') || fuzzySearch(query, cmd.section)
    );
  }, [query, allCommands]);

  useEffect(() => {
    inputRef.current?.focus();
  }, []);

  useEffect(() => {
    setSelectedIndex(0);
  }, [query]);
  
  useEffect(() => {
    const item = listRef.current?.children[selectedIndex] as HTMLLIElement;
    item?.scrollIntoView({ block: 'nearest' });
  }, [selectedIndex]);


  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        setIsCommandPaletteOpen(false);
      } else if (e.key === 'ArrowUp') {
        e.preventDefault();
        setSelectedIndex(prev => (prev > 0 ? prev - 1 : filteredCommands.length - 1));
      } else if (e.key === 'ArrowDown') {
        e.preventDefault();
        setSelectedIndex(prev => (prev < filteredCommands.length - 1 ? prev + 1 : 0));
      } else if (e.key === 'Enter') {
        e.preventDefault();
        const command = filteredCommands[selectedIndex];
        if (command) {
          command.onExecute();
        }
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [filteredCommands, selectedIndex, setIsCommandPaletteOpen]);

  const groupedCommands = useMemo(() => {
    return filteredCommands.reduce((acc, cmd) => {
        (acc[cmd.section] = acc[cmd.section] || []).push(cmd);
        return acc;
    }, {} as Record<string, Command[]>);
  }, [filteredCommands]);

  const commandListForIndexing = Object.values(groupedCommands).flat();

  return (
    <div 
        className="fixed inset-0 bg-black/50 z-50 flex justify-center items-start pt-24" 
        onClick={() => setIsCommandPaletteOpen(false)}
        aria-modal="true"
        role="dialog"
    >
      <div 
        className="w-full max-w-2xl bg-slate-800 border border-slate-700 rounded-lg shadow-2xl overflow-hidden flex flex-col"
        onClick={e => e.stopPropagation()}
      >
        <div className="flex items-center gap-3 p-4 border-b border-slate-700">
          <Icon name="search" className="w-5 h-5 text-slate-400" />
          <input
            ref={inputRef}
            type="text"
            placeholder="Type a command or search..."
            className="w-full bg-transparent text-lg text-white placeholder-slate-500 focus:outline-none"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
          />
           <div className="text-xs text-slate-500 bg-slate-700 px-2 py-1 rounded-md">
            ⌘ K
          </div>
        </div>
        <ul ref={listRef} className="max-h-[50vh] overflow-y-auto p-2">
          {filteredCommands.length > 0 ? (
            Object.entries(groupedCommands).map(([section, commands]) => (
                <li key={section}>
                    <div className="px-3 pt-3 pb-1 text-xs font-semibold text-slate-400 uppercase">{section}</div>
                    <ul>
                        {commands.map(cmd => {
                            const currentIndex = commandListForIndexing.findIndex(c => c.id === cmd.id);
                            return (
                                <li
                                    key={cmd.id}
                                    onClick={cmd.onExecute}
                                    className={`flex items-center justify-between gap-3 p-3 rounded-md cursor-pointer ${selectedIndex === currentIndex ? 'bg-slate-700' : 'hover:bg-slate-700/50'}`}
                                    onMouseMove={() => setSelectedIndex(currentIndex)}
                                    role="option"
                                    aria-selected={selectedIndex === currentIndex}
                                >
                                    <div className="flex items-center gap-3">
                                        <Icon name={cmd.icon} className="w-5 h-5 text-slate-300" />
                                        <span className="text-white">{cmd.name}</span>
                                    </div>
                                    {cmd.keywords && <span className="text-sm text-slate-500 truncate">{cmd.keywords}</span>}
                                </li>
                            )
                        })}
                    </ul>
                </li>
            ))
          ) : (
            <li className="p-8 text-center text-slate-500">No results found.</li>
          )}
        </ul>
      </div>
    </div>
  );
};
