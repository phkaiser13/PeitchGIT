import * as vscode from 'vscode';
import * as path from 'path';

/**
 * A utility class for providing themed icons.
 * This helps in maintaining a consistent look and feel.
 */
export class IconProvider {
    private static context: vscode.ExtensionContext;

    public static initialize(context: vscode.ExtensionContext) {
        this.context = context;
    }

    /**
     * Gets the URI for a specific icon.
     * @param iconName The name of the icon file (e.g., 'commit.svg').
     * @returns An object with light and dark theme URIs for the icon.
     */
    public static getIcon(iconName: string): { light: vscode.Uri, dark: vscode.Uri } {
        if (!this.context) {
            throw new Error("IconProvider must be initialized with the extension context.");
        }

        return {
            light: vscode.Uri.file(path.join(this.context.extensionPath, 'resources', 'light', iconName)),
            dark: vscode.Uri.file(path.join(this.context.extensionPath, 'resources', 'dark', iconName))
        };
    }

    /**
     * Gets a VS Code ThemeIcon.
     * @param iconId The ID of the ThemeIcon (e.g., 'git-commit').
     * @returns A VS Code ThemeIcon instance.
     */
    public static getThemeIcon(iconId: string): any {
        return { id: iconId };
    }
}