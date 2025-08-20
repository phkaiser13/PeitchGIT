import React, { useMemo } from 'react';
import { useAppContext } from '../../contexts/AppContext';
import { CommitGraph } from '../graph/CommitGraph';
import { processCommitsForGraph } from '../../utils/graphUtils';
import { useRepository } from '../../hooks/useRepository';

export const CommitGraphView: React.FC = () => {
    const { commits, branches, tags, selectedCommit } = useAppContext();
    const { selectCommit } = useRepository();

    const graphData = useMemo(() => {
        if (commits.length === 0) return { nodes: [], paths: [], decorations: [] };
        return processCommitsForGraph(commits, branches, tags);
    }, [commits, branches, tags]);

    if (!commits || commits.length === 0) {
        return <div className="p-4 text-slate-500">Loading commits...</div>;
    }
    
    return (
        <CommitGraph 
            graphData={graphData}
            selectedCommit={selectedCommit}
            onSelectCommit={selectCommit}
        />
    );
};
