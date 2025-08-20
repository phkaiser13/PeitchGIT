<div align="center" style="font-family:Segoe UI, Roboto, sans-serif;">
  <a href="https://github.com/phkaiser13/peitchgit"></a>

  <h1 style="font-size:2.5em; margin-bottom:0.2em;">⚙️ Peitch Installation Motor</h1>
  <p style="font-size:1.2em; color:#555; margin-top:0;">
    <em>A Data-Driven, Cross-Platform Engine for Smart Installations</em>
  </p>

  <p style="max-width:700px; font-size:1.05em; line-height:1.5em; color:#444;">
    This is the core engine behind the <code>Peitch</code> ecosystem.  
    A standalone, high-performance C++ application designed to provide a robust and intelligent installation experience across Windows, macOS, and Linux distributions.  
    It powers installers such as <code>PeitchGIT</code>, <code>PeitchGUI</code> and can be extended to other software.
  </p>

  <p>
    <a href="https://github.com/phkaiser13/peitchgit/blob/main/LICENSE">
      <img src="https://img.shields.io/badge/license-Apache--2.0-blue.svg?style=for-the-badge" alt="License" />
    </a>
    <a href="https://github.com/phkaiser13/peitchgit/actions/workflows/build-windows.yml">
      <img src="https://img.shields.io/github/actions/workflow/status/phkaiser13/peitchgit/build-windows.yml?branch=main&logo=windows&style=for-the-badge" alt="Windows Build Status" />
    </a>
    <a href="https://github.com/phkaiser13/peitchgit/actions/workflows/build-ubuntu.yml">
      <img src="https://img.shields.io/github/actions/workflow/status/phkaiser13/peitchgit/build-ubuntu.yml?branch=main&logo=ubuntu&style=for-the-badge" alt="Ubuntu Build Status" />
    </a>
    <a href="https://github.com/phkaiser13/peitchgit/actions/workflows/build-macos.yml">
      <img src="https://img.shields.io/github/actions/workflow/status/phkaiser13/peitchgit/build-macos.yml?branch=main&logo=apple&style=for-the-badge" alt="macOS Build Status" />
    </a>
  </p>
</div>

## Table of Contents

- [What is the Installation Motor?](#what-is-the-installation-motor)
- [Key Features](#-key-features)
- [Architectural Principles](#-architectural-principles)
- [How It Works](#-how-it-works)
- [Building the Installer](#-building-the-installer)
- [Configuration (`configjson`)](#-configuration-configjson)
- [Contributing](#-contributing)
- [License](#-license)

## What is the Installation Motor?

The **Peitch Installation Motor** is more than just a script.  
It’s an intelligent C++ engine responsible for installation and post-installation of toolchains and applications.  
It is designed as the smart core for native packages (DEB, RPM, MSI) or as a standalone executable, handling tasks such as dependency validation and dynamic component downloads.

## ✨ Key Features

- 🌐 **Cross-Platform by Design**: Unified C++17 codebase with platform-specific modules for **Windows**, **macOS**, and **Linux** families (Debian, Fedora, Arch).  
- ⚙️ **Data-Driven Configuration**: Behavior defined by `config.json`, not hardcoded. Easily update dependencies or metadata without recompilation.  
- 🚀 **Dynamic Dependency Management**: Detects required tools (Git, Terraform, Vault, etc.) and installs or updates them automatically.  
- 📡 **Live API Integration**: Connects to APIs (GitHub Releases, HashiCorp) to fetch latest binaries and secure checksums.  
- 🛡️ **Robust Security**: SHA256 checksum verification ensures file integrity.  
- 📦 **Flexible Strategies**: Uses native package managers (`apt`, `dnf`, `brew`) with fallback to official archives (`.tar.gz`, `.pkg`).  

## 🏛️ Architectural Principles

```mermaid
graph TD
    A[Installer Entry Point - main.cpp] --> B{PlatformDetector}
    B --> |PlatformInfo| C{DependencyManager}
    A --> D{ConfigManager}
    D --> E[config.json]
    D --> C
    D --> F{ApiManager}
    F --> G[Downloader - libcurl]
    G --> H[SHA256 Verifier]
    A --> I{Installer Factory}
    I --> J[IPlatformInstaller]
    
    subgraph "Platform-Specific Engines"
        J --> K[LinuxInstaller]
        J --> L[MacosInstaller]
        J --> M[WindowsInstaller]
    end

    K --> C
    L --> C
    M --> C
    K --> F
    L --> F
    M --> F
````

* **`main.cpp`**: Application entry point, orchestrates all steps.
* **`ConfigManager`**: Loads and parses `config.json`.
* **`PlatformDetector`**: Identifies OS, architecture, and privilege level.
* **`DependencyManager`**: Validates required external tools.
* **`ApiManager` & `Downloader`**: Fetch metadata and binaries securely.
* **`IPlatformInstaller`**: Contract for platform-specific strategies.

## ⚙️ How It Works

1. **Load Configuration** (`config.json`)
2. **Detect Platform** (OS, arch, privileges)
3. **Check Dependencies** (installed tools vs required versions)
4. **Instantiate Engine** (LinuxInstaller, WindowsInstaller, etc.)
5. **Run Installation** (native pkg managers or API fetch + checksum + integration)

## 🚀 Building the Installer

Requirements:

* C++17 compiler (GCC, Clang, MSVC)
* CMake ≥ 3.16
* `libcurl` development headers
* `spdlog` & `nlohmann/json` (via `FetchContent`)

```bash
cmake -S . -B build -DPEITCH_BUILD_INSTALLER=ON
cmake --build build
# Output: build/bin/peitch-installer
```

## 📜 Configuration (`config.json`)

Defines behavior via:

* **`package_metadata`**: Name, version, metadata.
* **`dependencies`**: Required and optional tools with minimum versions.
* **`api_endpoints`**: APIs for dynamic binary resolution.

## 🤝 Contributing

Contributions welcome!
See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## 📜 License

Distributed under the Apache License 2.0.
See [LICENSE](LICENSE).
