export interface Hunk {
    header: string;
    content: string; // The full content of the hunk, including header
}

export function parseDiff(diff: string): Hunk[] {
    const hunks: Hunk[] = [];
    if (!diff || !diff.trim()) return hunks;

    const lines = diff.split('\n');
    let currentHunkLines: string[] | null = null;

    for (const line of lines) {
        if (line.startsWith('@@')) {
            if (currentHunkLines) {
                hunks.push({ header: currentHunkLines[0], content: currentHunkLines.join('\n') });
            }
            currentHunkLines = [line];
        } else if (currentHunkLines) {
            currentHunkLines.push(line);
        }
    }

    if (currentHunkLines) {
        hunks.push({ header: currentHunkLines[0], content: currentHunkLines.join('\n') });
    }

    return hunks;
}
