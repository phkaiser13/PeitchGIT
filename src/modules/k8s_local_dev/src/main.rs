/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: src/modules/k8s_local_dev/src/main.rs
 *
 * This file serves as the main entry point for the `k8s_local_dev` command-line
 * interface (CLI). Its purpose is to provide developers with a simple, unified
 * tool to provision and manage local Kubernetes development environments using
 * various underlying provisioners like 'kind' (Kubernetes in Docker) and 'k3s'.
 * The entire application is built using the `clap` crate for powerful, declarative
 * argument parsing and the `tokio` runtime for asynchronous execution.
 *
 * Architecture:
 * The architecture is designed around `clap`'s derive-based argument parsing, which
 * allows for a highly structured and self-documenting CLI definition. The core
 * principles are:
 * 1. Declarative CLI Structure: Structs and enums are used with `clap` attributes
 * (`#[derive(Parser)]`, `#[command(...)]`, `#[arg(...)]`) to define the entire
 * command hierarchy, including subcommands, arguments, and help messages. This
 * approach is clean, readable, and less error-prone than manual parsing.
 * 2. Asynchronous Execution: The `main` function is marked with `#[tokio::main]`,
 * ensuring that all I/O operations, particularly the execution of external commands
 * (`kind`, `k3s`, etc.), are non-blocking. This is handled by dedicated functions
 * within the `provisioners` module.
 * 3. Modularity: The core logic for each provisioner is encapsulated in its own
 * module (e.g., `src/provisioners/kind.rs`, `src/provisioners/k3s.rs`). The `main.rs`
 * file is responsible only for parsing user input and dispatching the command to the
 * appropriate module, adhering to the single-responsibility principle.
 * 4. Rich User Feedback and Error Handling: The application provides clear feedback
 * to the user about the actions being performed. All fallible operations return
 * `anyhow::Result`, and errors are propagated up to the main function where they
 * are printed in a user-friendly format. This ensures that if an underlying tool
 * (like `kind`) fails, its error output is clearly visible.
 *
 * Process Flow:
 * - The user invokes the binary with a command, e.g., `k8s_local_dev create kind --name my-cluster`.
 * - `clap::Parser::parse()` is called on the `Cli` struct.
 * - `clap` parses the arguments and populates an instance of the `Cli` struct.
 * - A `match` statement on the `Cli.command` enum dispatches control to the handler
 * for the specified subcommand (e.g., `Commands::Create`).
 * - Inside the handler, another `match` statement on the provisioner enum dispatches
 * to the specific function (e.g., `provisioners::kind::create_cluster`).
 * - The provisioner function is called with the arguments provided by the user (e.g., cluster name).
 * - The provisioner function constructs and executes the necessary system command.
 * - The result (success or error) is propagated back up to `main` and the process exits.
 *
 * This structure makes the CLI easy to test and extend with new commands or provisioners
 * without modifying the core dispatch logic in `main.rs`.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

use anyhow::{Context, Result};
use clap::{Parser, Subcommand, ValueEnum};
use std::path::PathBuf;

// Import the provisioner modules. The actual implementation for kind, k3s, etc.,
// will reside in these files.
mod provisioners;

/// A unified CLI for managing local Kubernetes development environments.
#[derive(Parser, Debug)]
#[command(author, version, about, long_about = "A tool to streamline the creation and destruction of local Kubernetes clusters for development purposes using various provisioners.")]
struct Cli {
    /// The subcommand to execute.
    #[command(subcommand)]
    command: Commands,
}

/// Defines the available subcommands for the CLI.
#[derive(Subcommand, Debug)]
enum Commands {
    /// Creates a new local Kubernetes cluster.
    Create {
        /// The type of provisioner to use for creating the cluster.
        #[arg(value_enum)]
        provisioner: ProvisionerType,

        /// The name for the new cluster.
        #[arg(long, short, default_value = "dev")]
        name: String,

        /// Optional path to a provisioner-specific configuration file.
        #[arg(long, short)]
        config: Option<PathBuf>,
    },
    /// Deletes an existing local Kubernetes cluster.
    Delete {
        /// The type of provisioner that was used to create the cluster.
        #[arg(value_enum)]
        provisioner: ProvisionerType,

        /// The name of the cluster to delete.
        #[arg(long, short, default_value = "dev")]
        name: String,
    },
}

/// Enumerates the supported Kubernetes provisioners.
/// `ValueEnum` allows these to be parsed directly from the command line.
#[derive(ValueEnum, Debug, Clone, Copy, PartialEq, Eq)]
#[clap(rename_all = "kebab_case")]
enum ProvisionerType {
    /// Use 'kind' (Kubernetes in Docker) as the provisioner.
    Kind,
    /// Use 'k3s' as the provisioner.
    K3s,
}

/// The main entry point of the application, running on the Tokio async runtime.
#[tokio::main]
async fn main() -> Result<()> {
    // 1. Parse command-line arguments.
    // `Cli::parse()` will automatically handle argument validation, help text generation
    // (`--help`), and version information (`--version`). If parsing fails, it will
    // exit the process with a user-friendly error message.
    let cli = Cli::parse();

    // 2. Dispatch the command to the appropriate handler.
    // This `match` statement is the core of the application's control flow.
    match cli.command {
        Commands::Create { provisioner, name, config } => {
            println!("🚀 Starting cluster creation for '{}' using {:?}...", name, provisioner);

            // Dispatch based on the chosen provisioner.
            let result = match provisioner {
                ProvisionerType::Kind => {
                    provisioners::kind::create_cluster(&name, config.as_deref()).await
                }
                ProvisionerType::K3s => {
                    provisioners::k3s::create_cluster(&name, config.as_deref()).await
                }
            };

            // Handle the result of the operation.
            result.with_context(|| format!("Failed to create cluster '{}'", name))?;
            println!("\n✅ Cluster '{}' created successfully!", name);
        }
        Commands::Delete { provisioner, name } => {
            println!("🔥 Starting cluster deletion for '{}' using {:?}...", name, provisioner);

            // Dispatch based on the chosen provisioner.
            let result = match provisioner {
                ProvisionerType::Kind => provisioners::kind::delete_cluster(&name).await,
                ProvisionerType::K3s => provisioners::k3s::delete_cluster(&name).await,
            };

            // Handle the result of the operation.
            result.with_context(|| format!("Failed to delete cluster '{}'", name))?;
            println!("\n✅ Cluster '{}' deleted successfully!", name);
        }
    }

    Ok(())
}