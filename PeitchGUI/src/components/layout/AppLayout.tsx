import React from 'react';

interface AppLayoutProps {
  toolbar: React.ReactNode;
  sidebar: React.ReactNode;
  mainContent: React.ReactNode;
  detailsPanel: React.ReactNode;
  statusBar: React.ReactNode;
}

export const AppLayout: React.FC<AppLayoutProps> = ({ toolbar, sidebar, mainContent, detailsPanel, statusBar }) => {
  return (
    <div className="h-screen w-screen flex flex-col bg-slate-900">
      {toolbar}
      <div className="flex-grow grid grid-cols-[256px_1fr_minmax(450px,40%)] min-h-0">
        <div className="overflow-y-auto bg-slate-800/70 border-r border-slate-700">
          {sidebar}
        </div>
        <div className="overflow-hidden border-r border-slate-700">
          {mainContent}
        </div>
        <div className="overflow-y-auto">
          {detailsPanel}
        </div>
      </div>
      {statusBar}
    </div>
  );
};
