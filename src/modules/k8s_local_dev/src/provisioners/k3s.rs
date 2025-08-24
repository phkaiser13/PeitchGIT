/* Copyright (C) 2025 Pedro Henrique / phkaiser13
* File: src/modules/k8s_local_dev/src/provisioners/k3s.rs
* This file provides a placeholder implementation of the `Provisioner` trait for
* 'k3s', a lightweight Kubernetes distribution. The methods are intentionally
* left unimplemented, returning an error to indicate that this feature is
* planned but not yet available. This ensures the overall application structure
* is complete and compiles correctly.
* SPDX-License-Identifier: Apache-2.0 */

use super::Provisioner;
use anyhow::{anyhow, Result};
use async_trait::async_trait;

/// Represents the 'k3s' provisioner.
pub struct K3sProvisioner;

#[async_trait]
impl Provisioner for K3sProvisioner {
    /// Creates a 'k3s' cluster. (Not yet implemented)
    ///
    /// A real implementation would likely involve running the k3s installation
    /// script or managing the k3s server process.
    async fn create(&self, _name: &str, _k8s_version: &str) -> Result<()> {
        Err(anyhow!(
            "The 'k3s' provisioner is not yet implemented. Contribution is welcome!"
        ))
    }

    /// Deletes a 'k3s' cluster. (Not yet implemented)
    ///
    /// A real implementation would involve running the k3s uninstall script
    /// or stopping and removing the related systemd services.
    async fn delete(&self, _name: &str) -> Result<()> {
        Err(anyhow!(
            "The 'k3s' provisioner is not yet implemented. Contribution is welcome!"
        ))
    }

    /// Lists 'k3s' clusters. (Not yet implemented)
    ///
    /// A real implementation would check for running k3s processes or
    /// systemd services.
    async fn list(&self) -> Result<()> {
        Err(anyhow!(
            "The 'k3s' provisioner is not yet implemented. Contribution is welcome!"
        ))
    }
}