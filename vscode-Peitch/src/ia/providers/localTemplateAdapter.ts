import { IAProvider, GenerateOptions } from "../types";

export class LocalTemplateAdapter implements IAProvider {
    id = 'local';
    name = 'Local Template';

    async testConnection(apiKey: string): Promise<boolean> {
        // No connection needed for local templates
        return true;
    }

    async generate(prompt: string, apiKey: string, options?: GenerateOptions): Promise<string[]> {
        // Mock implementation: returns a conventional commit based on the prompt
        const firstLine = prompt.split('\n')[0].toLowerCase();
        let type = 'feat';
        if (firstLine.includes('fix') || firstLine.includes('bug')) type = 'fix';
        if (firstLine.includes('refactor')) type = 'refactor';
        
        return [
            `${type}: apply requested changes`,
            `chore: update files based on diff`,
        ];
    }
}
