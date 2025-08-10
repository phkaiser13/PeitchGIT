# gitph User Guide

## 1\. Introduction

Welcome to `gitph`\!

`gitph` is not just another Git wrapper; it's a powerful, polyglot command-line assistant designed to supercharge your development and DevOps workflows. Whether you prefer a straightforward command-line interface (CLI) for scripting and automation or a guided, menu-driven text-based UI (TUI) for interactive sessions, `gitph` has you covered.

The core philosophy behind `gitph` is leveraging the best tool for the job. It combines the raw performance and low-level control of **C** and **Rust**, the powerful concurrency of **Go** for network tasks, and a flexible **Lua** scripting engine for user-level customization.

This guide will walk you through everything from installation to advanced usage and customization.

## 2\. Getting Started

### 2.1. Prerequisites

Before building `gitph` from source, you need to ensure you have the necessary development toolchains installed. The project provides a helper script to check for these dependencies.

**Required Dependencies:**

  * A C/C++ compiler (like GCC or Clang)
  * CMake (version 3.15 or higher)
  * Go (version 1.18 or higher)
  * The Rust toolchain (rustc and cargo)
  * Lua 5.4 development headers (`liblua5.4-dev` on Debian/Ubuntu)
  * libcurl development headers (`libcurl4-openssl-dev` on Debian/Ubuntu)

On Debian-based Linux systems, you can install most of these with a single command:

```bash
sudo apt-get install build-essential cmake pkg-config liblua5.4-dev libcurl4-openssl-dev nlohmann-json3-dev
```

*Note: This command is taken from the CI workflow and is a reliable way to get the core C/C++ dependencies.*

For other systems (Fedora, Arch, macOS), a setup script is available at `scripts/setup_dev_env.sh` which will attempt to use your native package manager to install these dependencies.

### 2.2. Building the Project

`gitph` uses CMake to orchestrate its complex multi-language build process, with a convenient `Makefile` for simpler commands.

1.  **Clone the repository:**

    ```bash
    git clone https://github.com/phkaiser13/peitchgit.git
    cd peitchgit
    ```

2.  **Build the application:**
    The easiest way to build is by using the provided `Makefile`:

    ```bash
    make
    ```

    This command will automatically run CMake to configure the project and then build all components in parallel.

Alternatively, you can use the build scripts directly:

  * On Linux/macOS: `./scripts/build.sh`
  * On Windows: `.\scripts\build.bat`

After a successful build, you will find the main executable at `build/bin/gitph` and all the functional modules as shared libraries (`.so` or `.dll`) in `build/bin/modules/`.

### 2.3. Configuration

`gitph` can be configured using a simple `key=value` file named `gitph.conf` located in the same directory as the executable. The application will load this file on startup.

The configuration manager ignores empty lines and lines starting with a `#` symbol.

**Example `gitph.conf`:**

```
# This is a comment
default_remote=origin
default_branch=develop
```

Modules can access these values to modify their behavior. For instance, the `git_ops` module could be extended to use `default_remote` and `default_branch` for the `SND` command.

## 3\. Core Concepts

### 3.1. Dual Interface: TUI and CLI

`gitph` is designed for two primary modes of operation.

**Interactive Mode (TUI):**
If you run `gitph` without any arguments, it will launch a Text-based User Interface. This mode is perfect for new users or for exploring the available features. The TUI dynamically generates a menu of all available commands by querying the loaded modules, ensuring the menu is always up-to-date with the installed plugins.

**Command-Line Mode (CLI):**
You can also call `gitph` directly with commands and arguments, making it ideal for scripting and automation.

```bash
./build/bin/gitph status
```

The CLI dispatcher is the central router for all non-interactive commands. It finds the appropriate module to handle the command and executes it.

### 3.2. The Polyglot Module System

The power of `gitph` comes from its modules. The C core application is intentionally lightweight; all the major features are implemented in self-contained modules written in Rust, Go, or C++. These modules are loaded dynamically at runtime from the `modules/` directory. This means you can add or update features simply by adding or replacing a module file, without recompiling the core application.

## 4\. Module Command Reference

This section provides a detailed guide to the commands offered by each core module.

### 4.1. `git_ops` (Rust)

This module provides fundamental Git operations by wrapping the `git` command-line tool. The logic is implemented to be robust and provide clear feedback.

  * **`status`**

      * **Description:** Shows a concise, machine-readable view of the current repository status. It uses `git status --porcelain` internally for a clean output.
      * **Usage:** `gitph status`
      * **Output:** If there are changes, it will print the modified files. If the working tree is clean, it will print a "Working tree clean" message.

  * **`SND`**

      * **Description:** A powerful high-level "Send" command that automates a common workflow. It automatically:
        1.  Stages all current changes (`git add .`).
        2.  Commits them with a default message ("Automated commit from gitph").
        3.  Pushes the commit to the default remote and branch (`git push origin main`).
      * **Usage:** `gitph SND`
      * **Note:** This is an opinionated command designed for speed. It will intelligently handle cases where there is nothing to commit.

### 4.2. `api_client` (Go)

A client module for interacting with Git provider APIs, such as GitHub. It is built using Go for its excellent networking and JSON handling capabilities.

  * **`srp <user/repo>`**
      * **Description:** "Set RePository". This command fetches and displays public metadata about a GitHub repository, including stars, forks, description, and the default branch.
      * **Usage:** `gitph srp phkaiser13/peitchgit`
      * **Output:** A formatted table displaying the fetched repository information.

### 4.3. `issue_tracker` (Rust)

An asynchronous client for interacting with issue tracking services like GitHub Issues. It uses Rust's `tokio` runtime to perform network requests without blocking the main application, ensuring the UI remains responsive.

  * **`issue-get <user/repo> <issue_id>`**
      * **Description:** Fetches and displays the details for a specific issue from a GitHub repository.
      * **Usage:** `gitph issue-get rust-lang/rust 50000`
      * **Output:** A clean, formatted summary of the issue including its ID, title, author, state, and URL.

### 4.4. `devops_automation` (Go)

A wrapper for common DevOps command-line tools like Terraform and Vault, designed to streamline Infrastructure-as-Code (IaC) and secret management workflows.

  * **`tf-plan`**

      * **Description:** Executes `terraform plan` and streams the output directly to your console in real-time. This is ideal for interactive review of planned infrastructure changes.
      * **Usage:** `gitph tf-plan`

  * **`tf-apply`**

      * **Description:** Executes `terraform apply` for interactive IaC workflows, again with real-time streaming output.
      * **Usage:** `gitph tf-apply`

  * **`vault-read <secret_path>`**

      * **Description:** Reads a secret from HashiCorp Vault. It captures the JSON output, parses it, and displays the secret's key-value pairs in a readable format.
      * **Usage:** `gitph vault-read secret/data/app/config`
      * **Output:** A list of the key-value data stored at that secret path.

### 4.5. `sync_engine` (Rust)

This is one of `gitph`'s most advanced features. It's a sophisticated, stateful engine for performing bi-directional repository synchronization. Unlike simple `git pull/push` wrappers, it uses the `git2-rs` library to directly manipulate the Git object database for complex commit graph analysis.

  * **`sync-run <path_to_source_repo> <path_to_target_repo>`**
      * **Description:** Initiates the synchronization process between two local repositories.
      * **Stateful Operation:** The engine is stateful. After the first successful sync, it creates a file at `.git/gitph_sync_state.json` inside the source repository to remember the last common commit. This allows for more intelligent syncs in the future.
      * **Divergence Detection:** The engine's primary safety feature is its ability to detect "divergence"—a situation where both repositories have unique commits since the last sync. If divergence is detected, the engine will safely abort and require manual intervention to prevent accidental data loss.
      * **Usage:** `gitph sync-run ./my-repo-fork ./official-repo`

### 4.6. `ci_cd_manager` (Go/C++)

This is a two-part module for visualizing CI/CD workflows. While its direct user-facing command may evolve, its internal mechanism is a showcase of polyglot architecture.

  * **Functionality:** A Go component parses a workflow YAML file (like a GitHub Actions `.yml` file) and serializes it to a language-agnostic JSON string. This JSON is then passed to a C++ component that parses the JSON and prints a formatted, human-readable representation of the pipeline to the console.

## 5\. Extending `gitph` with Lua Plugins

One of the most powerful features of `gitph` is its extensibility. You can add your own custom functionality by placing simple Lua scripts (`.lua` files) into the `src/plugins/` directory. The C core will automatically load and execute these scripts at startup.

A global table named `gitph` is exposed to the Lua environment, providing access to core functionalities.

### 5.1. Lua API Functions

  * `gitph.log(level, message)`

      * **Description:** Allows your plugin to write to the main `gitph` log file (`gitph_log.txt`). The `level` should be a string: "INFO", "DEBUG", "WARN", or "ERROR".
      * **Example:** `gitph.log("INFO", "My custom plugin has loaded.")`

  * `gitph.register_alias(alias, command)`

      * **Description:** This is the key to creating your own shortcuts. It registers a new `alias` that, when run, will execute the target `command`.
      * **Example:** The included `custom_aliases.lua` plugin demonstrates this perfectly:
        ```lua
        -- Allows you to run 'gitph st' instead of 'gitph status'
        gitph.register_alias("st", "status")

        -- Allows you to run 'gitph snd' instead of 'gitph SND'
        gitph.register_alias("snd", "SND")
        ```

### 5.2. Event Hooks

You can define special global functions in your Lua scripts that the `gitph` core will automatically call at specific points in its lifecycle. This enables you to enforce policies or automate actions.

  * `on_pre_push(remote, branch)`
      * **Description:** If you define a global function with this exact name, it will be executed by the core just before a push operation. You can perform checks and, crucially, **return `false` to cancel the push**.
      * **Example:** The included `pre_push_hook.lua` plugin provides an excellent example of policy enforcement. It prevents direct pushes to the `main` branch:
        ```lua
        function on_pre_push(remote, branch)
            gitph.log("INFO", "Executing pre-push hook...")

            -- Policy: Prevent direct pushes to the 'main' branch.
            if branch == "main" then
                gitph.log("ERROR", "Direct push to 'main' branch is forbidden.")
                print("[POLICY VIOLATION] You cannot push directly to the main branch.")
                return false -- This cancels the push operation.
            end

            -- If all checks pass, allow the push.
            return true
        end
        ```
