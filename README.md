<div align="center">
  <br />
  <h1>📖 Ph Git (Peitch Git)</h1>
  <strong>The Polyglot Assistant for Git & DevOps Workflows</strong>
  <br />
  <br />
  <p>
    A modern, extensible, and high-performance command-line tool designed to unify and streamline your development lifecycle.
  </p>
  <img src="https://img.shields.io/badge/license-Apache-2.0-blue.svg" alt="license" />
  <img src="https://img.shields.io/github/v/release/phkaiser13/peitchgit" alt="release" />
  <img src="https://img.shields.io/badge/contributions-welcome-brightgreen.svg" alt="contributions welcome" />
</div>


-----

**`gitph`** reimagines the developer's command line by providing a single, cohesive interface for Git, DevOps tooling, and workflow automation. Built on a unique **polyglot architecture**, it acts as a lightweight orchestrator, leveraging the strengths of C, C++, Rust, and Go to deliver unparalleled performance and safety.

With a dual CLI and interactive TUI, and a powerful Lua scripting engine for unlimited extensibility, `gitph` is designed to be the last workflow tool you'll ever need.

### Core Architecture: The Best Tool for the Job

`gitph`'s foundation is a lean **C Core** that dynamically loads and orchestrates a suite of modules. This allows each component to be written in the language best suited for its task, ensuring optimal performance, safety, and development velocity.

```
+------------------------------------------------------+
|                 User (CLI / TUI)                     |
+--------------------------+---------------------------+
                           |
                           v
+--------------------------+---------------------------+
|                  C CORE (Orchestrator)               |
|  (Module Loader, CLI Parser, Lua Bridge, Config Mgr) |
+--------------------------+---------------------------+
                           | (Loads Modules via C FFI)
                           v
+------------+-------------+-------------+-------------+
| C++ Module | Rust Modules| Go Modules  | Lua Plugins |
|------------|-------------|-------------|-------------|
| - Visualizer | - Git Ops   | - API Client| - Aliases   |
| - Logger     | - Sync Engine | - DevOps    | - Hooks     |
|            | - Issue Tracker | - CI Parser |             |
+------------+-------------+-------------+-------------+
```

## ✨ Key Features

  * ⚡️ **Unified Workflow Automation**: Execute complex, multi-step operations like staging, committing, and pushing with a single, intelligent command (`gitph SND`).
  * ⚙️ **Powerful Bi-Directional Sync**: Go beyond standard Git with a stateful synchronization engine that uses low-level repository analysis to manage complex mirroring and update workflows safely.
  * 🛠️ **Seamless DevOps Integration**: Interact with essential tools like Terraform and Vault directly through the `gitph` interface, with support for both interactive streaming and data capture.
  * 🔌 **Deep Extensibility with Lua**: Don't just use `gitph`—remake it. Add custom aliases, automate tasks, and enforce team policies with powerful, easy-to-write Lua scripts and event hooks.
  * 📡 **Intelligent API Clients**: Fetch data from GitHub, GitLab, and other services with asynchronous, non-blocking clients that keep the UI responsive.
  * 🖥️ **Dual CLI & Interactive TUI**: Whether you're a power user scripting in the shell or a newcomer exploring features, `gitph` provides both a full-featured CLI and a discoverable, menu-driven TUI.

## The `gitph` Philosophy

Modern development involves more than just Git. It's a web of CI/CD pipelines, infrastructure-as-code, issue trackers, and provider APIs. `gitph` was born from the idea that these tools shouldn't require context-switching. By acting as a central orchestrator, `gitph` unifies these disparate workflows under one roof.

Our polyglot approach is a deliberate engineering choice to ensure excellence at every level:

| Language | Role & Rationale |
| :--- | :--- |
| **C** | **The Orchestrator.** Provides low-level control, dynamic module loading, and maximum portability for the application's core lifecycle. |
| **C++** | **High-Performance Libraries.** Powers components like the thread-safe logger and CI/CD visualizer, using modern C++ features for robustness and safe resource management (RAII). |
| **Rust** | **Memory Safety & Critical Performance.** The choice for mission-critical modules like the `sync_engine` and `issue_tracker`, where correctness, safety, and speed are non-negotiable. |
| **Go** | **Concurrency Made Simple.** Perfect for network-bound tasks and CLI wrappers like the `api_client` and `devops_automation` modules, thanks to Go's elegant concurrency model and rich standard library. |
| **Lua** | **User-Level Scripting.** Embedded to empower users with the ability to define custom hooks, aliases, and personalized workflows without needing to recompile the application. |

## 🚀 Getting Started

### Prerequisites

Ensure the following dependencies are installed. We provide helper scripts to verify and install them.

  * `gcc`/`g++` (or `clang`)
  * `cmake` (version 3.15+)
  * `go` (version 1.18+)
  * `rustc` and `cargo`
  * Lua 5.4 and libcurl development headers

**Automated Setup (Linux & macOS):**

```bash
./scripts/setup_dev_env.sh
```

This script will detect your package manager and attempt to install all required dependencies.

### Build Instructions

The project uses a `Makefile` as a convenient wrapper around its CMake build system.

1.  **Clone the repository:**

    ```bash
    git clone https://github.com/phkaiser13/peitchgit.git
    cd peitchgit
    ```

2.  **Build the project:**

    ```bash
    make
    ```

    The main executable will be located at `build/bin/gitph`, with all modules in `build/bin/modules/`.

## 💻 Usage Showcase

### The "One-Shot" Send

Tired of `git add .`, `git commit -m "..."`, `git push`? Automate it.

```bash
# Stage, commit, and push all changes in one command
./build/bin/gitph SND
```

`gitph` handles the entire sequence, even detecting if there's nothing to commit.

### Interactive Mode

Simply run `gitph` with no arguments to enter a menu-driven TUI that lists all available commands from all loaded modules.

```bash
./build/bin/gitph
```

### DevOps Integration

Read a secret from Vault without wrestling with JSON output.

```bash
# Reads the secret and displays it as clean key-value pairs
./build/bin/gitph vault-read secret/data/prod/api-keys
```

### Issue Tracking

Quickly check an issue's status directly from your terminal.

```bash
./build/bin/gitph issue-get "golang/go" "58532"
```

## 🔌 Extending with Lua

Create your own commands and hooks by adding `.lua` files to the `src/plugins/` directory.

**Example 1: Custom Aliases**

Create `src/plugins/my_aliases.lua`:

```lua
-- Create a short alias 'st' for the 'status' command
gitph.register_alias("st", "status")

-- Log to the main gitph log file to confirm loading
gitph.log("INFO", "Loaded custom alias 'st'.")
```

Now, `gitph st` works just like `gitph status`\!

**Example 2: Enforce Team Policy with a Hook**

Create `src/plugins/policy.lua` to prevent direct pushes to the `main` branch:

```lua
-- This global function is automatically called by gitph before a push
function on_pre_push(remote, branch)
    if branch == "main" then
        print("[POLICY] Direct pushes to the main branch are forbidden.")
        print("Please use a pull request.")
        return false -- This cancels the push operation
    end
    -- Allow pushes to any other branch
    return true
end
```

This powerful hook system allows you to integrate linters, run tests, or enforce any workflow rule before code leaves your machine.

## 🤝 Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1.  Fork the Project
2.  Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3.  Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4.  Push to the Branch (`git push origin feature/AmazingFeature`)
5.  Open a Pull Request

Please feel free to open an issue for any bugs or feature requests using our templates.

## 📜 License

Distributed under the Apache-2.0. See `LICENSE` for more information.
