<div align="center" style="font-family:Segoe UI, Roboto, sans-serif;">
  <a href="https://github.com/phkaiser13/peitchgit">
    </a>

  <h1 style="font-size:2.5em; margin-bottom:0.2em;">⚙️ Ph Git Installer Engine</h1>
  <p style="font-size:1.2em; color:#555; margin-top:0;">
    <em>A Data-Driven, Cross-Platform Installer for Modern Tooling</em>
  </p>

  <p style="max-width:700px; font-size:1.05em; line-height:1.5em; color:#444;">
    This is the core engine for the `phgit` installer. It is a standalone, high-performance C++ application designed to provide a robust and intelligent installation experience across Windows, macOS, and various Linux distributions.
  </p>
</div>


## Table of Contents

- [What is the Installer Engine?](#what-is-the-installer-engine)
- [Key Features](#-key-features)
- [Architectural Principles](#️-architectural-principles)
- [How It Works](#-how-it-works)
- [Building the Installer](#-building-the-installer)
- [Configuration (`config.json`)](#-configuration-configjson)
- [Contributing](#-contributing)
- [License](#-license)

## What is the Installer Engine?

The `phgit` installer is more than just a script that copies files. It's an intelligent C++ application responsible for the post-installation or standalone setup of the `phgit` toolchain. It is designed to be the smart core inside native packages (like DEB, RPM, MSI) or to be run as a standalone executable, handling tasks that require logic and network access, such as dependency validation and dynamic component downloads.

## ✨ Key Features

-   🌐 **Cross-Platform by Design**: A single C++ codebase with platform-specific modules to handle the nuances of **Windows**, **macOS**, and major **Linux** distribution families (Debian, Fedora, Arch).
-   ⚙️ **Data-Driven Configuration**: The installer's behavior is defined by `config.json`, not hardcoded. This allows for easy updates to dependencies, API endpoints, and metadata without recompiling the engine.
-   🚀 **Dynamic Dependency Management**: Automatically detects required tools like Git, Terraform, and Vault. If they are missing or outdated, it can fetch and install them from their official sources.
-   📡 **Live API Integration**: Communicates with external APIs (GitHub Releases, HashiCorp) to fetch the latest versions and checksums for dependencies, ensuring users always get up-to-date and secure components.
-   🛡️ **Robust and Secure**: Performs SHA256 checksum verification on all downloaded files to ensure integrity and prevent corruption or tampering.
-   📦 **Flexible Installation Strategies**: Leverages native package managers (`apt`, `dnf`, `brew`) when available, with a robust fallback to installing from a `.tar.gz` or `.pkg` for maximum compatibility.

## 🏛️ Architectural Principles

The installer is built on a modular, object-oriented architecture that separates concerns, making it easy to maintain and extend.

```mermaid
graph TD
    A[Installer Entry Point (main.cpp)] --> B{PlatformDetector};
    B --> |PlatformInfo| C{DependencyManager};
    A --> D{ConfigManager};
    D -- reads --> E[config.json];
    D --> C;
    D --> F{ApiManager};
    F --> G[Downloader (libcurl)];
    G --> H[SHA256 Verifier];
    A --> I{Installer Factory};
    I -- creates --> J[IPlatformInstaller];
    subgraph "Platform-Specific Engines"
        J ---> K[LinuxInstaller];
        J ---> L[MacosInstaller];
        J ---> M[WindowsInstaller];
    end

    K -- uses --> C;
    L -- uses --> C;
    M -- uses --> C;
    K -- uses --> F;
    L -- uses --> F;
    M -- uses --> F;
