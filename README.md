---

# 📖 gitph — The Polyglot Git Assistant

**gitph** is a modern and extensible command-line interface (CLI) tool designed to streamline and supercharge your Git and DevOps workflows. Built on a unique **polyglot architecture**, it leverages the raw performance of **C/C++** and **Rust**, the concurrency of **Go**, and the scripting flexibility of **Lua** to deliver a high-performance, customizable experience.

---

## ✨ Key Features

* **Dual Interface:** Use `gitph` directly via CLI for scripting or switch to **interactive mode (TUI)** for a guided, menu-driven experience.
* **Simplified Git Commands:** Execute complex operations using smart aliases like `gitph SND`, which automatically stages, commits, and pushes your changes.
* **Advanced Sync Engine:** A powerful bidirectional sync engine that manipulates the Git repository at a low level, enabling operations beyond standard Git CLI capabilities.
* **DevOps Automation:** Wrap tools like **Terraform** and **Vault** for seamless integration and secure IaC workflows.
* **Issue Tracking Integration:** View GitHub issue details directly from your terminal.
* **CI/CD Workflow Viewer:** Parse CI workflows (e.g., GitHub Actions) and visually explore pipelines from the terminal.
* **Highly Extensible:** Add your own aliases and automation hooks using embedded **Lua scripting** with minimal effort.

---

## 🛠️ Polyglot Architecture

`gitph` is a real-world case study in polyglot software design. Each language is chosen for its strengths in specific domains, resulting in a modular, scalable, and efficient system.

| Language     |                                             Icon                                             | Role                                                                                                                                                                              |
| ------------ | :------------------------------------------------------------------------------------------: | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **C**        |       <img src="https://img.icons8.com/color/48/000000/c-programming.png" width="24"/>       | **The Orchestrator.** Acts as the core of the application, offering low-level control, dynamic module loading, and lifecycle orchestration with maximum portability.              |
| **C++**      |      <img src="https://img.icons8.com/color/48/000000/c-plus-plus-logo.png" width="24"/>     | **High-Performance Libraries.** Powers components such as the thread-safe logger and CI/CD visualizer, utilizing modern C++ features and RAII for robustness.                     |
| **Rust**     | <img src="https://img.icons8.com/color/48/000000/rust-programming-language.png" width="24"/> | **Memory Safety & Critical Performance.** Used for critical modules like the `sync_engine` and async `issue_tracker` client, where safety and speed are non-negotiable.           |
| **Go**       |           <img src="https://img.icons8.com/color/48/000000/golang.png" width="24"/>          | **Concurrency Made Simple.** Ideal for API clients and CLI wrappers such as `api_client` and `devops_automation`, thanks to Go’s elegant concurrency model and rapid development. |
| **Lua**      |        <img src="https://img.icons8.com/color/48/000000/lua-language.png" width="24"/>       | **User-Level Scripting.** Embedded as a scripting engine to enable user-defined hooks, custom aliases, and personalized automation with minimal overhead.                         |
| **Protobuf** |        <img src="https://img.icons8.com/color/48/000000/google-logo.png" width="24"/>        | **Language-Agnostic Contracts.** Facilitates efficient communication between Go and C++ components through well-defined binary data structures.                                   |

---

## 🚀 Getting Started

### Prerequisites

To build `gitph`, make sure the following dependencies are installed:

* `gcc`/`g++` (or `clang`)
* `cmake` (version 3.15+)
* `go` (version 1.18+)
* `rustc` and `cargo`
* `liblua5.4-dev` (Lua headers)
* `libcurl4-openssl-dev` (C URL library headers)

On Debian/Ubuntu systems:

```bash
sudo apt-get install build-essential cmake golang rustc liblua5.4-dev libcurl4-openssl-dev
```

---

### Build Instructions

`gitph` uses **CMake** as its primary build system, with a convenience `Makefile`.

1. **Clone the repository:**

```bash
git clone https://github.com/your-username/gitph.git
cd gitph
```

2. **Build the project:**

Quick build:

```bash
make
```

Step-by-step:

```bash
make configure  # Run once to generate build files
make build
```

3. **Post-build:**

* The main executable will be located in `build/bin/gitph`.
* All dynamic modules (`.so` or `.dll`) will be in `build/bin/modules/`.

---

## 💻 Usage

### Command-Line Mode (CLI)

Execute commands directly for scripting and automation:

```bash
# View repository status
./build/bin/gitph status

# Stage, commit, and push all changes in one command
./build/bin/gitph SND

# Fetch issue details from GitHub
./build/bin/gitph issue-get <user/repo> <issue_id>
```

### Interactive Mode (TUI)

Run `gitph` with no arguments to launch the terminal UI:

```bash
./build/bin/gitph
```

---

## 🔌 Lua-Based Extensibility

You can extend `gitph` by adding `.lua` scripts in the `src/plugins/` directory.

**Example: `src/plugins/custom_aliases.lua`**

```lua
-- Define a shorthand 'st' for the 'status' command
if gitph.register_alias then
  gitph.register_alias("st", "status")
  gitph.log("INFO", "Alias 'st' for 'status' registered!")
end
```

---

## 🤝 Contributing

We welcome contributions! To get involved:

1. Fork the repository.
2. Create a feature branch: `git checkout -b feature/YourFeature`.
3. Commit your changes: `git commit -m 'Add YourFeature'`.
4. Push to your branch: `git push origin feature/YourFeature`.
5. Open a pull request.

---

## 📜 License

This project is licensed under the **GNU General Public License v3.0**.
See the [LICENSE](./LICENSE) file for full details.

---
