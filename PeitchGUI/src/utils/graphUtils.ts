import type { Commit, Branch, Tag } from '../types';

export interface GraphNode {
  commit: Commit;
  x: number;
  y: number;
  color: string;
}

export interface GraphPath {
  d: string;
  color: string;
}

export interface GraphDecoration {
  name: string;
  sha: string;
  type: 'branch' | 'tag';
}

export interface ProcessedGraph {
  nodes: GraphNode[];
  paths: GraphPath[];
  decorations: GraphDecoration[];
}

const branchColors = ['#6366f1', '#ec4899', '#10b981', '#f59e0b', '#3b82f6', '#8b5cf6', '#ef4444', '#22c55e'];

const getColor = (name: string) => {
  let hash = 0;
  for (let i = 0; i < name.length; i++) {
    hash = name.charCodeAt(i) + ((hash << 5) - hash);
  }
  return branchColors[Math.abs(hash) % branchColors.length];
};

const ROW_HEIGHT = 40;
const LANE_WIDTH = 20;

export function processCommitsForGraph(
  commits: Commit[],
  branches: Branch[],
  tags: Tag[]
): ProcessedGraph {
  const nodes: GraphNode[] = [];
  const paths: GraphPath[] = [];
  const decorations: GraphDecoration[] = [
      ...branches.map(b => ({ name: b.name, sha: b.sha, type: 'branch' as const })),
      ...tags.map(t => ({ name: t.name, sha: t.sha, type: 'tag' as const }))
  ];

  const commitMap = new Map<string, Commit>(commits.map(c => [c.sha, c]));
  const commitY = new Map<string, number>(commits.map((c, i) => [c.sha, i]));
  
  let lanes: (string | null)[] = [];
  const commitLanes = new Map<string, number>();

  commits.forEach((commit, y) => {
    let lane = commitLanes.get(commit.sha);
    if (lane === undefined) {
      lane = lanes.indexOf(null);
      if (lane === -1) {
        lane = lanes.length;
      }
    }
    lanes[lane] = commit.sha;
    commitLanes.set(commit.sha, lane);
    
    const color = getColor(commit.branch || lane.toString());

    nodes.push({ commit, x: lane, y, color });
    
    // Process parents
    commit.parents.forEach((parentSha, parentIndex) => {
        const parentCommit = commitMap.get(parentSha);
        if(!parentCommit) return;
        
        const parentY = commitY.get(parentSha);
        if(parentY === undefined) return;

        let parentLane = commitLanes.get(parentSha);
        if(parentLane === undefined) {
             // Parent is further down the list, it will draw its own path up
             if (parentIndex === 0) {
                 commitLanes.set(parentSha, lane!);
             } else {
                 let newLane = lanes.indexOf(null);
                 if (newLane === -1) { newLane = lanes.length; }
                 lanes[newLane] = parentSha;
                 commitLanes.set(parentSha, newLane);
             }
        }
        
        parentLane = commitLanes.get(parentSha)!;
        
        // Draw path from child (current commit) to parent
        const x1 = lane! * LANE_WIDTH + LANE_WIDTH / 2;
        const y1 = y * ROW_HEIGHT + ROW_HEIGHT / 2;
        const x2 = parentLane * LANE_WIDTH + LANE_WIDTH / 2;
        const y2 = parentY * ROW_HEIGHT + ROW_HEIGHT / 2;

        const d = `M ${x1} ${y1} C ${x1} ${y1 + ROW_HEIGHT / 2}, ${x2} ${y2 - ROW_HEIGHT / 2}, ${x2} ${y2}`;
        paths.push({ d, color });
    });

    // Free up lanes
    const currentBranches = commits.slice(0, y+1).map(c => c.branch);
    lanes.forEach((sha, i) => {
        if(sha) {
            const c = commitMap.get(sha)!;
            const isHead = branches.some(b => b.sha === sha);
            const children = commits.filter(child => child.parents.includes(sha));
            if (children.length === 0 && !isHead) {
               // lanes[i] = null;
            }
        }
    });

  });

  return { nodes, paths, decorations };
}
