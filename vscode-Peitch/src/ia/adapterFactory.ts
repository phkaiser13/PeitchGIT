import { IAProvider } from "./types";
import { OpenAIAdapter } from "./providers/openaiAdapter";
import { GeminiAdapter } from "./providers/geminiAdapter";
import { LocalTemplateAdapter } from "./providers/localTemplateAdapter";

const providers: { [key: string]: IAProvider } = {
    'openai': new OpenAIAdapter(),
    'gemini': new GeminiAdapter(),
    'local': new LocalTemplateAdapter(),
};

export function getAdapter(providerId: string): IAProvider | undefined {
    return providers[providerId];
}

export function getAvailableProviders(): { id: string, name: string }[] {
    return Object.values(providers).map(p => ({ id: p.id, name: p.name }));
}
