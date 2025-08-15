# Getting Started with gitph

This guide will walk you through the basic usage of `gitph`, from running your first command to using its powerful interactive mode.

## Command Structure

`gitph` follows a standard command-line structure:

```
gitph [COMMAND] [ARGUMENTS...]
```

- `COMMAND`: The name of the action you want to perform (e.g., `SND`, `status`, `vault-read`).
- `ARGUMENTS`: Additional information required by the command (e.g., a commit message, a file path).

You can see a list of all available commands by running `gitph --help` or by launching the interactive TUI.

## Interactive TUI Mode

If you run `gitph` without any arguments, you will enter a Text-based User Interface (TUI).

```bash
gitph
```

This mode is a great way to explore `gitph`'s capabilities. It provides a menu of all loaded commands from every module, with descriptions of what they do. You can navigate the menu with your arrow keys and press `Enter` to execute a command. The TUI will then prompt you for any required arguments.

## Core Commands Showcase

Here are a few examples of `gitph`'s key features in action.

### The "One-Shot" Send (`SND`)

The `SND` command is a powerful workflow enhancement that automates the standard `git add .`, `git commit -m "..."`, and `git push` cycle. It's intelligent enough to know when there are no changes to commit, preventing empty commits.

**Usage:**

```bash
gitph SND "Your commit message here"
```

**How it works:**

1.  `gitph` stages all unstaged changes in your current directory (`git add .`).
2.  It commits the staged changes with the message you provided.
3.  It pushes the commit to the configured upstream branch.
4.  If there are no changes to commit, it will inform you and exit gracefully.

This single command saves you time and streamlines the process of getting your local work pushed to your remote repository.

### Securely Read Secrets (`vault-read`)

The `devops_automation` module allows `gitph` to interact with other command-line tools and parse their output. A great example is the `vault-read` command, which securely reads a secret from HashiCorp Vault and displays it in a clean, human-readable format.

**Usage:**

```bash
gitph vault-read secret/data/prod/api-keys
```

**How it works:**

- Instead of just running `vault kv get -format=json ...` and giving you raw JSON, `gitph` captures the output.
- It parses the JSON data internally and displays the secret's key-value pairs, making it easy to find the information you need without piping the output to tools like `jq`.

### Safe Repository Synchronization (`sync-run`)

The `sync-engine` provides a stateful, bi-directional synchronization mechanism that is safer than simple `git mirror` or `rsync` operations. It's designed to prevent accidental data loss when syncing repositories that may have diverged.

**Usage:**

```bash
gitph sync-run /path/to/source-repo /path/to/target-repo
```

**How it works:**

1.  The `sync-engine` analyzes the commit history of both the source and target repositories.
2.  It creates a state file inside the source repository's `.git` directory to track sync operations.
3.  It detects if the branches have diverged (i.e., if both have unique commits).
4.  If divergence is detected, it will warn you and prevent a sync that could overwrite history, prompting you to manually resolve the divergence first.

This makes `sync-run` a powerful tool for maintaining forks or backups of a repository.
