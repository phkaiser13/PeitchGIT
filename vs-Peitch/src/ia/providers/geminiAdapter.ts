import { IAProvider, GenerateOptions } from "../types";

export class GeminiAdapter implements IAProvider {
    id = 'gemini';
    name = 'Google Gemini';

    async testConnection(apiKey: string): Promise<boolean> {
        // Mock implementation: always returns true if key is not empty
        return !!apiKey;
    }

    async generate(prompt: string, apiKey: string, options?: GenerateOptions): Promise<string[]> {
        console.log(`[Gemini Mock] Generating with prompt starting with: ${prompt.substring(0, 100)}...`);

        if (prompt.includes('Generate a changelog')) {
            const commitsRaw = prompt.split('---')[1] || '';
            const commits = commitsRaw.trim().split('\n');

            const features = commits.filter(c => c.match(/^(feat|feature)/i)).map(c => `- ${c}`);
            const fixes = commits.filter(c => c.match(/^(fix|bugfix)/i)).map(c => `- ${c}`);
            const chores = commits.filter(c => c.match(/^(chore|build|ci|docs|refactor)/i)).map(c => `- ${c}`);

            let changelog = '';
            if (features.length > 0) {
                changelog += `### Features\n${features.join('\n')}\n\n`;
            }
            if (fixes.length > 0) {
                changelog += `### Fixes\n${fixes.join('\n')}\n\n`;
            }
            if (chores.length > 0) {
                changelog += `### Chores\n${chores.join('\n')}\n\n`;
            }
            
            return [changelog.trim() || 'No categorized commits found.'];
        }

        // Mock implementation for commit messages based on diff
        const addCount = (prompt.match(/^\+/gm) || []).length;
        const removeCount = (prompt.match(/^-/gm) || []).length;

        const suggestions = [];
        if (addCount > 5) suggestions.push("feat: implement new functionality and UI components");
        if (removeCount > 5) suggestions.push("refactor: simplify and remove obsolete code");
        if (addCount < 5 && removeCount < 5 && addCount > 0) suggestions.push("fix: address minor issue in component logic");
        suggestions.push("chore: update project files and dependencies");
        suggestions.push("docs: add detailed instructions to contribution guide");
        
        return suggestions.slice(0, 3);
    }
}
