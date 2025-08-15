# Installation

`gitph` is designed to be built from source on a variety of platforms. We also offer pre-built packages for several popular package managers to simplify the installation process.

## Option 1: Installing from a Package Manager (Recommended)

Using a package manager is the easiest way to get `gitph` up and running.

### Homebrew (macOS)

If you are on macOS and use Homebrew, you can install `gitph` from our official tap:

```bash
brew install phkaiser13/peitchgit/gitph
```

### Chocolatey (Windows)

If you are on Windows and use Chocolatey, you can install `gitph` with the following command:

```bash
choco install gitph
```

### Arch User Repository (Arch Linux)

For Arch Linux users, `gitph` is available on the AUR. You can use an AUR helper like `yay` to install it:

```bash
yay -S gitph
```

## Option 2: Building from Source

If a package is not available for your system, or if you want to build the latest development version, you can build `gitph` from source.

### Prerequisites

Before you begin, you need to install the necessary development toolchain for your platform. `gitph` is a polyglot project and requires:

- A modern C/C++ compiler (e.g., GCC, Clang, MSVC)
- `cmake` (version 3.15 or newer)
- `git`
- The Rust toolchain (`rustc` and `cargo`)
- Development headers for `libcurl` and `lua`

We provide a convenience script to help set up this environment on some systems.

```bash
# On Debian/Ubuntu-based systems
sudo apt-get update
sudo apt-get install build-essential cmake git curl libcurl4-openssl-dev liblua5.4-dev

# On Fedora/RHEL-based systems
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git curl libcurl-devel lua-devel

# On macOS (using Homebrew)
brew install cmake git rust lua

# For Rust, we recommend installing via rustup
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Build Steps

Once the prerequisites are installed, you can clone the repository and build the project using our `Makefile` wrapper, which simplifies the `cmake` process.

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/phkaiser13/peitchgit.git
    cd peitchgit
    ```

2.  **Build the project:**
    The `make` command will configure the project with `cmake` and then compile the entire application.
    ```bash
    make
    ```
    This will create a `build` directory containing the compiled binaries.

3.  **Run `gitph`:**
    The main executable will be located at `build/bin/gitph`.
    ```bash
    ./build/bin/gitph --version
    ```

4.  **(Optional) Install the binary:**
    To make `gitph` available system-wide, you can install it to a location in your `PATH`.
    ```bash
    sudo make install
    ```
    This will typically install the binary to `/usr/local/bin`.
