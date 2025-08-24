/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/k8s_local_dev/src/main.rs
* This is the main entry point for the `k8s_local_dev` command-line executable.
* Its primary responsibility is to parse command-line arguments, use the
* provisioner factory to select the appropriate backend, and dispatch the
* command to the provisioner's implementation. It orchestrates the application
* flow and handles top-level error reporting.
* SPDX-License-Identifier: Apache-2.0 */

mod cli;
mod provisioners;

use crate::cli::{Cli, Commands};
use anyhow::Result;
use clap::Parser;

/// The main entry point for the asynchronous application.
#[tokio::main]
async fn main() {
    // Execute the core application logic. If it returns an error, print it
    // to stderr and exit with a non-zero status code, which is standard
    // practice for CLI tools.
    if let Err(e) = run().await {
        eprintln!("Error: {:?}", e);
        std::process::exit(1);
    }
}

/// Contains the core logic of the CLI application.
async fn run() -> Result<()> {
    // Parse the command-line arguments into our strongly-typed `Cli` struct.
    let cli = Cli::parse();

    // Use a `match` statement to dispatch the appropriate logic for each subcommand.
    // This is the central orchestration point of the application.
    match cli.command {
        Commands::Create(args) => {
            println!(
                "Received 'create' command for cluster '{}' using provider '{:?}'...",
                args.cluster_name, args.provider
            );
            // Get the appropriate provisioner implementation using the factory.
            let provisioner = provisioners::get_provisioner(args.provider)?;
            // Execute the 'create' action.
            provisioner
                .create(&args.cluster_name, &args.k8s_version)
                .await?;
        }
        Commands::Delete(args) => {
            println!(
                "Received 'delete' command for cluster '{}' using provider '{:?}'...",
                args.cluster_name, args.provider
            );
            let provisioner = provisioners::get_provisioner(args.provider)?;
            provisioner.delete(&args.cluster_name).await?;
        }
        Commands::List(args) => {
            println!(
                "Received 'list' command for provider '{:?}'...",
                args.provider
            );
            let provisioner = provisioners::get_provisioner(args.provider)?;
            provisioner.list().await?;
        }
    }

    println!("\nOperation completed successfully.");
    Ok(())
}