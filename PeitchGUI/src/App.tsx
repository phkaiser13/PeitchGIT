import React, { useEffect } from 'react';
import { HomePage } from './pages/HomePage';
import { useRepository } from './hooks/useRepository';
import { useAppContext } from './contexts/AppContext';
import { CommandPalette } from './components/shared/CommandPalette';
import { DailySummaryModal } from './components/shared/DailySummaryModal';

const App: React.FC = () => {
  const { loadInitialData } = useRepository();
  const { isCommandPaletteOpen, setIsCommandPaletteOpen, isSummaryModalOpen } = useAppContext();

  useEffect(() => {
    loadInitialData();
  }, [loadInitialData]);

  // Global key listener for command palette
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault();
        setIsCommandPaletteOpen(prev => !prev);
      }
    };
    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [setIsCommandPaletteOpen]);

  return (
    <>
      <HomePage />
      {isCommandPaletteOpen && <CommandPalette />}
      {isSummaryModalOpen && <DailySummaryModal />}
    </>
  );
};

export default App;
