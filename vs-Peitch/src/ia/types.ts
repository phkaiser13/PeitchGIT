/**
 * Options for generating text with an AI provider.
 */
export interface GenerateOptions {
    style?: 'short' | 'conventional' | 'long';
    ticketId?: string;
}

/**
 * Interface for an AI provider adapter.
 */
export interface IAProvider {
  id: string;
  name: string;
  testConnection(apiKey: string): Promise<boolean>;
  generate(prompt: string, apiKey: string, options?: GenerateOptions): Promise<string[]>; // Returns multiple suggestions
}
