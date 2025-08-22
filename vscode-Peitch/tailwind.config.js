/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    "./src/ui/**/*.tsx",
  ],
  theme: {
    extend: {
      colors: {
        'vscode-bg': 'var(--vscode-sideBar-background)',
        'vscode-text': 'var(--vscode-editor-foreground)',
        'vscode-text-secondary': 'var(--vscode-descriptionForeground)',
        'vscode-border': 'var(--vscode-sideBar-border)',
        'vscode-input-bg': 'var(--vscode-input-background)',
        'vscode-button-bg': 'var(--vscode-button-background)',
        'vscode-button-hover-bg': 'var(--vscode-button-hoverBackground)',
        'vscode-list-hover-bg': 'var(--vscode-list-hoverBackground)',
        'vscode-list-active-bg': 'var(--vscode-list-activeSelectionBackground)',
        'git-added': 'var(--vscode-gitDecoration-addedResourceForeground)',
        'git-modified': 'var(--vscode-gitDecoration-modifiedResourceForeground)',
        'git-deleted': 'var(--vscode-gitDecoration-deletedResourceForeground)',
        'git-untracked': 'var(--vscode-gitDecoration-untrackedResourceForeground)',
        'git-conflicting': 'var(--vscode-gitDecoration-conflictingResourceForeground)',
      }
    },
  },
  plugins: [],
}
