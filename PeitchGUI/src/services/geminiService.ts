import { GoogleGenAI, Type } from "@google/genai";
import type { AIProvider, Commit, SecretScanResult } from "../types";

// This service now handles multiple AI providers.

async function generateWithGemini(diff: string, model: string): Promise<string> {
  const prompt = `
    Based on the following code changes (git diff), generate a concise and descriptive commit message following the Conventional Commits specification.
    The message should start with a type (e.g., feat, fix, chore, docs, refactor), followed by an optional scope, a colon, and a short, imperative-tense description.
    Provide only the commit message text, without any extra formatting or explanation.

    Diff:
    ---
    ${diff}
    ---
  `;

  try {
    // Per guidelines, API key must come from environment variables for Gemini.
    const ai = new GoogleGenAI({ apiKey: process.env.API_KEY });

    const response = await ai.models.generateContent({
      model: model,
      contents: prompt,
      config: {
        temperature: 0.4,
        topP: 1,
        topK: 32,
        maxOutputTokens: 100,
      }
    });
    return response.text.trim();
  } catch (error) {
    console.error("Error generating commit message with Gemini:", error);
    if (error instanceof Error) {
        throw new Error(`Gemini suggestion failed. ${error.message}`);
    }
    throw new Error("Gemini suggestion failed due to an unknown error.");
  }
}

async function generateWithOpenAI(diff: string, model: string, apiKey: string): Promise<string> {
    // Placeholder for OpenAI implementation
    console.log(`Calling OpenAI with model ${model} and diff.`);
    if (!apiKey) {
        throw new Error("OpenAI API key is not configured.");
    }
    // In a real implementation, you would use fetch() to call the OpenAI API.
    return new Promise(resolve => setTimeout(() => resolve(`feat: Implement feature based on OpenAI's analysis`), 500));
}

async function generateWithAnthropic(diff: string, model: string, apiKey: string): Promise<string> {
    // Placeholder for Anthropic implementation
    console.log(`Calling Anthropic with model ${model} and diff.`);
     if (!apiKey) {
        throw new Error("Anthropic API key is not configured.");
    }
    // In a real implementation, you would use fetch() to call the Anthropic API.
    return new Promise(resolve => setTimeout(() => resolve(`fix: Correct behavior as suggested by Anthropic`), 500));
}

export async function generateCommitMessage(
    diff: string, 
    provider: AIProvider, 
    model: string, 
    apiKey: string
): Promise<string> {
    switch (provider) {
        case 'gemini':
            return generateWithGemini(diff, model);
        case 'openai':
            return generateWithOpenAI(diff, model, apiKey);
        case 'anthropic':
            return generateWithAnthropic(diff, model, apiKey);
        default:
            throw new Error(`Unsupported AI provider: ${provider}`);
    }
}

export async function generateDailySummary(commits: Commit[], authorName: string): Promise<string> {
    const commitLog = commits.map(c => `- "${c.message}" affecting files: ${c.files.map(f => f.path).join(', ')}`).join('\n');
    const prompt = `
        You are a helpful assistant for a software developer.
        Based on the following list of git commits from '${authorName}' today, generate a concise summary of the work accomplished.
        Structure the summary in markdown format with headings for key features or fixes, and bullet points for details.
        The tone should be professional and suitable for a daily stand-up or a status report.

        Commit Log:
        ---
        ${commitLog}
        ---
    `;

    try {
        const ai = new GoogleGenAI({ apiKey: process.env.API_KEY });
        const response = await ai.models.generateContent({
            model: 'gemini-2.5-flash',
            contents: prompt,
            config: {
                temperature: 0.5,
                maxOutputTokens: 500,
            }
        });
        return response.text.trim();
    } catch (error) {
        console.error("Error generating daily summary with Gemini:", error);
        throw new Error("Failed to generate daily summary.");
    }
}


export async function scanForSecrets(diffs: {path: string, diff: string}[]): Promise<SecretScanResult> {
    const diffContent = diffs.map(d => `--- a/${d.path}\n+++ b/${d.path}\n${d.diff}`).join('\n\n');
    const prompt = `
        You are an automated security scanner. Your task is to analyze the following git diff for any potential secrets like API keys, passwords, private keys, or other sensitive credentials.
        Only analyze newly added lines (starting with '+').
        If you find any potential secrets, list them. If not, indicate that no secrets were found.
        
        Git Diff:
        ---
        ${diffContent}
        ---
    `;

    try {
        const ai = new GoogleGenAI({ apiKey: process.env.API_KEY });
        const response = await ai.models.generateContent({
            model: 'gemini-2.5-flash',
            contents: prompt,
            config: {
                responseMimeType: "application/json",
                responseSchema: {
                    type: Type.OBJECT,
                    properties: {
                        found: {
                            type: Type.BOOLEAN,
                            description: "Whether any potential secrets were found."
                        },
                        issues: {
                            type: Type.ARRAY,
                            description: "A list of potential secrets found.",
                            items: {
                                type: Type.OBJECT,
                                properties: {
                                    path: {
                                        type: Type.STRING,
                                        description: "The file path where the issue was found."
                                    },
                                    reason: {
                                        type: Type.STRING,
                                        description: "A brief explanation of why this is a potential secret."
                                    }
                                }
                            }
                        }
                    }
                }
            }
        });
        const jsonString = response.text.trim();
        return JSON.parse(jsonString) as SecretScanResult;
    } catch (error) {
        console.error("Error scanning for secrets with Gemini:", error);
        throw new Error("Failed to scan for secrets.");
    }
}
