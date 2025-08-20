import React, { useState, useCallback } from 'react';
import type { ProcessedGraph } from '../../utils/graphUtils';
import type { Commit } from '../../types';
import { Icon } from '../shared/Icon';
import { useRepository } from '../../hooks/useRepository';
import { ContextMenu, type ContextAction } from '../shared/ContextMenu';

interface CommitGraphProps {
  graphData: ProcessedGraph;
  selectedCommit: Commit | null;
  onSelectCommit: (commit: Commit) => void;
}

const ROW_HEIGHT = 40;
const LANE_WIDTH = 20;
const NODE_RADIUS = 4;

export const CommitGraph: React.FC<CommitGraphProps> = ({ graphData, selectedCommit, onSelectCommit }) => {
  const { createBranch, createTag, cherryPickCommit } = useRepository();
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; commit: Commit } | null>(null);

  const handleContextMenu = useCallback((event: React.MouseEvent, commit: Commit) => {
    event.preventDefault();
    setContextMenu({ x: event.clientX, y: event.clientY, commit });
  }, []);

  const closeContextMenu = useCallback(() => {
    setContextMenu(null);
  }, []);

  const getContextMenuActions = (commit: Commit): ContextAction[] => [
    {
      label: 'Create branch from here...',
      icon: 'git-branch-plus',
      onClick: () => {
        const branchName = prompt(`Create new branch from commit ${commit.shortSha}:`);
        if (branchName) {
          createBranch(branchName, commit.sha);
        }
      },
    },
    {
      label: 'Create tag...',
      icon: 'tag',
      onClick: () => {
        const tagName = prompt(`Create new tag at commit ${commit.shortSha}:`);
        if (tagName) {
          createTag(tagName, commit.sha);
        }
      },
    },
    { isSeparator: true },
    {
      label: 'Cherry-pick commit',
      icon: 'copy',
      onClick: () => {
        if (window.confirm(`Are you sure you want to cherry-pick commit ${commit.shortSha}?\n\n"${commit.message}"`)) {
          cherryPickCommit(commit);
        }
      },
    },
  ];

  if (!graphData || graphData.nodes.length === 0) {
    return <div className="p-4 text-slate-500">No commits to display.</div>;
  }

  const { nodes, paths, decorations } = graphData;
  const height = nodes.length * ROW_HEIGHT;
  const width = (Math.max(...nodes.map(n => n.x)) + 2) * LANE_WIDTH;

  return (
    <div className="h-full overflow-y-auto bg-slate-900 relative">
      <svg width={width} height={height} className="absolute top-0 left-0">
        <defs>
            <linearGradient id="fade-out" x1="0%" y1="0%" x2="0%" y2="100%">
                <stop offset="0%" style={{stopColor: 'black', stopOpacity: 0.2}} />
                <stop offset="10%" style={{stopColor: 'black', stopOpacity: 0}} />
            </linearGradient>
        </defs>
        <g>
          {paths.map((path, index) => (
            <path key={index} d={path.d} stroke={path.color} strokeWidth="2" fill="none" />
          ))}
          {nodes.map(node => (
            <circle
              key={node.commit.sha}
              cx={node.x * LANE_WIDTH + LANE_WIDTH / 2}
              cy={node.y * ROW_HEIGHT + ROW_HEIGHT / 2}
              r={NODE_RADIUS}
              fill={node.color}
              stroke={selectedCommit?.sha === node.commit.sha ? '#38bdf8' : node.color}
              strokeWidth={selectedCommit?.sha === node.commit.sha ? 2 : 0}
              className="cursor-pointer transition-all"
              onClick={() => onSelectCommit(node.commit)}
              onContextMenu={(e) => handleContextMenu(e, node.commit)}
            />
          ))}
        </g>
      </svg>
      <table className="w-full text-sm border-separate border-spacing-0 relative">
        <thead className="sticky top-0 bg-slate-900/80 backdrop-blur-sm z-10">
          <tr>
            <th style={{ minWidth: width, width: width }} className="text-left p-2 font-semibold border-b border-slate-700">Graph</th>
            <th className="w-1/2 text-left p-2 font-semibold border-b border-slate-700">Description</th>
            <th className="w-1/4 text-left p-2 font-semibold border-b border-slate-700">Commit</th>
          </tr>
        </thead>
        <tbody>
          {nodes.map((node, index) => {
            const { commit } = node;
            const isSelected = selectedCommit?.sha === commit.sha;
            return (
              <tr 
                key={commit.sha}
                onClick={() => onSelectCommit(commit)}
                onContextMenu={(e) => handleContextMenu(e, commit)}
                className={`cursor-pointer transition-colors duration-100 ${isSelected ? 'bg-sky-500/10' : 'hover:bg-slate-800/50'}`}
                style={{ height: ROW_HEIGHT }}
              >
                <td className="p-2 border-b border-slate-800 align-middle relative">
                  <div className="absolute top-1/2 -translate-y-1/2 flex items-center gap-2 flex-wrap">
                    {decorations.filter(d => d.sha === commit.sha).map(d => (
                      <span 
                        key={d.name}
                        className={`text-xs font-semibold px-2 py-0.5 rounded-full text-white flex items-center gap-1 ${d.type === 'branch' ? 'bg-blue-600' : 'bg-amber-500'}`}
                      >
                        <Icon name={d.type === 'branch' ? 'git-branch' : 'tag'} className="w-3 h-3" />
                        {d.name}
                      </span>
                    ))}
                  </div>
                </td>
                <td className="p-2 border-b border-slate-800">
                  <div className="font-medium text-slate-100 truncate">{commit.message}</div>
                  <div className="text-slate-400">{commit.author.name}</div>
                </td>
                <td className="p-2 border-b border-slate-800">
                  <div className="font-mono text-slate-400">{commit.shortSha}</div>
                  <div className="text-slate-500">{new Date(commit.date).toLocaleDateString()}</div>
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
      {contextMenu && (
        <ContextMenu
          x={contextMenu.x}
          y={contextMenu.y}
          actions={getContextMenuActions(contextMenu.commit)}
          onClose={closeContextMenu}
        />
      )}
    </div>
  );
};