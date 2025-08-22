import { IAProvider, GenerateOptions } from "../types";

export class OpenAIAdapter implements IAProvider {
    id = 'openai';
    name = 'OpenAI';

    async testConnection(apiKey: string): Promise<boolean> {
        // Mock implementation: always returns true if key is not empty
        return !!apiKey;
    }

    async generate(prompt: string, apiKey: string, options?: GenerateOptions): Promise<string[]> {
        console.log(`[OpenAI Mock] Generating with prompt starting with: ${prompt.substring(0, 100)}...`);
        
        if (prompt.includes('Generate a changelog')) {
            const commitsRaw = prompt.split('---')[1] || '';
            const commits = commitsRaw.trim().split('\n');
            const changelogItems = commits.map(c => `* ${c.replace(/^(feat|fix|chore|docs|refactor)[\w]*: /i, '')}`);

            const changelog = `## What's Changed\n${changelogItems.join('\n')}`;
            return [changelog];
        }

        // Mock implementation for commit messages
        const suggestions = [];
        if (prompt.includes('import React')) {
            suggestions.push("feat(ui): add new React component");
        }
        if (prompt.includes('package.json')) {
             suggestions.push("chore: update project dependencies");
        }
        suggestions.push("refactor: simplify component logic for readability");
        suggestions.push("fix: correct typo in variable name");
        
        return suggestions.slice(0, 3);
    }
}
