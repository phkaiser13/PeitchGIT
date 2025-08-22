<div align="center" style="font-family:Segoe UI, Roboto, sans-serif;">
  <a href="https://github.com/phdev13/peitchgit">
  </a>

  <h1 style="font-size:2.5em; margin-bottom:0.2em;">Peitch Git for VS Code</h1>
  <p style="font-size:1.2em; color:#555; margin-top:0;">
    <em>The official VS Code plugin for the Ph Git command-line toolchain.</em>
  </p>

  <p style="max-width:700px; font-size:1.05em; line-height:1.5em; color:#444;">
    Bring the power of the polyglot <code>phgit</code> engine directly into your editor. This plugin provides a rich, interactive interface to manage your repositories, orchestrate complex workflows, and leverage advanced features like AI-powered releases and multi-remote management without ever leaving VS Code.
  </p>

  <p>
    <a href="https://marketplace.visualstudio.com/items?itemName=phdev13.peitch-git">
      <img src="https://img.shields.io/visual-studio-marketplace/v/phdev13.peitch-git?style=for-the-badge&label=VS%20Marketplace" alt="VS Marketplace Version" />
    </a>
    <a href="https://github.com/phdev13/peitchgit/blob/main/LICENSE">
      <img src="https://img.shields.io/badge/license-Apache--2.0-blue.svg?style=for-the-badge" alt="License" />
    </a>
    <a href="https://github.com/phdev13/peitchgit/actions">
      <img src="https://img.shields.io/github/actions/workflow/status/phdev13/peitchgit/build-phgit.yml?branch=main&logo=github&style=for-the-badge" alt="Build Status" />
    </a>
  </p>
</div>


-----

## ✨ Key Features

  * **Integrated Git Interface**: Visualize your repository history, manage branches, tags, remotes, and worktrees from a dedicated and intuitive sidebar view.
  * **Effortless Staging & Committing**: A dedicated panel for viewing diffs, staging/unstaging files with a single click, and crafting the perfect commit without command-line guesswork.
  * **Zero-Checkout Branch Inspection**: Instantly browse the complete file tree of any branch without performing a `checkout`, saving you time and context-switching.
  * **🤖 AI-Powered Release Management**: Automate your release process. Generate detailed changelogs from your commit history using AI, create annotated tags, and push releases with a single command.
  * **Advanced Workflow Tools**: Seamlessly manage Git worktrees, push to multiple mirror remotes simultaneously, and access the full suite of `phgit` commands directly from the Command Palette (`Ctrl+Shift+P`).

-----

## 🚀 Getting Started

### Installation

1.  Open **Visual Studio Code**.
2.  Go to the **Extensions** view (`Ctrl+Shift+X`).
3.  Search for "**Peitch Git**".
4.  Click **Install**.

Once installed, the Peitch Git icon will appear in your Activity Bar.

-----

## 💻 UI Showcase

### 1\. The Peitch Sidebar

Click the Peitch icon in the Activity Bar to get a complete overview of your repository. From here, you can navigate your branches, remotes, tags, and worktrees. Clicking any item will open a detailed view in the main panel.

### 2\. Commits & Staging Panel

When you open the main panel, the **Overview** tab shows your current branch status. Switch to the **Commits & Staging** tab to see all modified files.

  * Use the `+` icon to **stage** a file.
  * Use the `-` icon to **unstage** it.
  * Write your commit message and click **Commit** to finalize.

### 3\. The Branch Inspector

In the sidebar, simply click on any branch name. The main panel will switch to the **Branch View**, showing the entire file tree for that branch. You can open and view files as if you had checked out the branch, which is perfect for quick reviews and comparisons.

-----

## 🛠️ Advanced Features & Commands

Access powerful workflows directly from the Command Palette (`Ctrl+Shift+P` or `Cmd+Shift+P`):

  * **`Peitch: Create Release with AI`**: Kicks off the automated release process. It analyzes commits between two refs (e.g., tags or branches), generates a changelog, creates a new annotated tag, and pushes it.
  * **`Peitch: Push to All Remotes`**: Pushes your current branch to all repositories configured in your `peitch.mirrors` setting. Ideal for keeping forks and backups synchronized.
  * **`Peitch: Manage Worktrees`**: A UI-driven way to create, list, and remove Git worktrees, enabling you to work on multiple branches simultaneously without stashing changes.
  * **`Peitch: Add Remote`**: Quickly add a new remote repository.

-----

## ⚙️ Configuration

Customize Peitch Git's behavior via your VS Code `settings.json` file.

```json
{
  // Default branch for comparisons and release generation.
  "peitch.defaultBranch": "main",

  // Interval in minutes for automatic background fetching. Set to 0 to disable.
  "peitch.autoFetchIntervalMin": 5,

  // Allow the extension to send commit data to AI providers for changelog generation.
  "peitch.allowSendToIA": true,

  // A list of remote names to push to when using the "Push to All Remotes" command.
  "peitch.mirrors": ["origin", "backup", "dev-server"],

  // (Future) Configure deployment targets for CI/CD integration.
  "peitch.deployTargets": {
    "staging": "deploy_staging_script.sh",
    "production": "deploy_production_script.sh"
  }
}
```

-----

## 🧪 Development & Testing

Interested in contributing? Here’s how to get the extension running locally.

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/phdev13/peitchgit.git
    cd peitchgit/vscode-extension # Navigate to the extension directory
    ```
2.  **Install dependencies**:
    ```bash
    npm install
    ```
3.  **Start the development session**:
      * Open the `vscode-extension` folder in VS Code.
      * Press `F5` to open a new **Extension Development Host** window with the extension loaded.
4.  **Run tests**:
    ```bash
    npm test
    ```
    This command will execute the Jest test suite and report the results.

-----

## 📜 License

This extension is distributed under the terms of the **Apache License 2.0**.

See the [LICENSE](https://github.com/phdev13/peitchgit/blob/main/LICENSE) file for more information.