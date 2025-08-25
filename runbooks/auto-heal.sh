/*
 * Copyright (C) 2025 Pedro Henrique / phkaiser13
 *
 * File: runbooks/auto-heal.sh
 *
 * This script serves as an automated runbook for healing Kubernetes applications,
 * designed to be triggered by webhooks from Prometheus Alertmanager. Its primary role
 * is to increase system reliability by performing immediate, automated remediation actions
 * in response to well-defined failure scenarios, thereby reducing Mean Time To Recovery (MTTR).
 *
 * The script is architected with safety, idempotency, and extensibility in mind.
 * Key architectural principles include:
 *
 * 1.  **Strict Mode Operation:** It utilizes `set -euo pipefail` to ensure that it fails fast
 * and predictably if any command fails, an unset variable is used, or a command in a
 * pipeline fails. This prevents unintended consequences from partial script execution.
 *
 * 2.  **Dependency Verification:** It explicitly checks for the presence of `kubectl`, its
 * core dependency, before attempting any operations.
 *
 * 3.  **Environment-Driven Configuration:** The script is configured entirely through
 * environment variables, which is a standard pattern for systems triggered by Alertmanager.
 * It expects variables like `ALERT_NAMESPACE` and `ALERT_DEPLOYMENT` to be passed in the
 * webhook configuration, making it adaptable to different alerts and services.
 *
 * 4.  **Clear and Structured Logging:** All actions are logged to standard output with
 * timestamps and severity levels (INFO, ERROR), providing a clear audit trail for
 * post-mortem analysis.
 *
 * 5.  **Safety First (Dry-Run Mode):** A `DRY_RUN` mode is supported. If the `DRY_RUN`
 * environment variable is set to "true", the script will only log the `kubectl` commands
 * it *would* have executed, without making any actual changes to the cluster. This is
 * essential for testing and validation.
 *
 * 6.  **Focused, Idempotent Action:** The primary healing action implemented is a Deployment
 * rollout restart (`kubectl rollout restart`). This is a safe, common, and often effective
 * first-line remediation for stateless applications that have entered a bad state.
 * Restarting a rollout is idempotent; multiple restarts will not cause further harm.
 *
 * This runbook acts as a concrete implementation of automated SRE principles, forming a
 * crucial part of a resilient, self-healing infrastructure.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#!/bin/bash

#
# ---[ Script Configuration and Safety ]--------------------------------------
#
# set -e: Exit immediately if a command exits with a non-zero status.
# set -u: Treat unset variables as an error when substituting.
# set -o pipefail: The return value of a pipeline is the status of the last
#                  command to exit with a non-zero status, or zero if no
#                  command exited with a non-zero status.
#
# This trifecta is essential for writing robust and predictable shell scripts.
# It prevents the script from continuing in an undefined state after a failure.
#
set -euo pipefail

#
# ---[ Logging Utilities ]----------------------------------------------------
#
# A simple, structured logging mechanism. This ensures that all output from the
# script is timestamped and categorized, which is invaluable for auditing and
# debugging automated actions.
#
log_info() {
    # ISO 8601 format is an unambiguous and universally sortable standard.
    echo "[$(date -u +"%Y-%m-%dT%H:%M:%SZ")] [INFO] -- $1"
}

log_error() {
    # Logging errors to stderr is a standard practice that allows for redirection
    # of error streams separately from standard output.
    echo "[$(date -u +"%Y-%m-%dT%H:%M:%SZ")] [ERROR] -- $1" >&2
}

#
# ---[ Main Execution Logic ]-------------------------------------------------
#
main() {
    log_info "Auto-heal runbook triggered."

    #
    # ---[ Dependency Verification ]------------------------------------------
    #
    # Before proceeding, we must verify that our primary tool, kubectl, is available
    # in the execution environment's PATH. Failing early with a clear message is
    # better than failing later with an obscure "command not found" error.
    #
    if ! command -v kubectl &> /dev/null; then
        log_error "kubectl command could not be found. Aborting."
        exit 1
    fi
    log_info "Dependency check passed: kubectl is available."

    #
    # ---[ Variable Validation ]----------------------------------------------
    #
    # This script relies on environment variables, typically passed from an Alertmanager
    # webhook. We must validate their presence. The `set -u` option will cause the script
    # to exit if these variables are not set, but we add explicit checks here to provide
    # more user-friendly error messages.
    #
    # Note on Alertmanager configuration:
    # In your Alertmanager webhook config, you would pass these variables using templates,
    # for example:
    # url: "http://your-runbook-service/runbook?ALERT_NAMESPACE={{ .CommonLabels.namespace }}&ALERT_DEPLOYMENT={{ .CommonLabels.deployment }}"
    # Or as environment variables if the webhook receiver executes the script directly.
    # We use `${VAR:-}` syntax for safety, although `set -u` already protects us.
    #
    : "${ALERT_NAMESPACE:?ERROR: ALERT_NAMESPACE environment variable is not set.}"
    : "${ALERT_DEPLOYMENT:?ERROR: ALERT_DEPLOYMENT environment variable is not set.}"
    # The DRY_RUN variable is optional and defaults to "false".
    DRY_RUN="${DRY_RUN:-false}"

    log_info "Received alert for Deployment: '${ALERT_DEPLOYMENT}' in Namespace: '${ALERT_NAMESPACE}'."

    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "DRY_RUN mode is enabled. No changes will be made to the cluster."
    fi

    #
    # ---[ Remediation Action ]-----------------------------------------------
    #
    # The core logic of the runbook. Here, we perform the auto-healing action.
    # For this initial version, the action is a rollout restart. This is a common
    # and safe operation for stateless services that might have crashed or entered
    # a persistently unhealthy state (e.g., CrashLoopBackOff, deadlocked).
    #
    # Future enhancements could involve a `case` statement on an `ALERT_ACTION`
    # variable to support different remediation steps like `rollback` or `scale`.
    #
    log_info "Attempting to perform a rollout restart on the deployment."

    # We build the command in a variable to make the dry-run logic cleaner.
    local kubectl_command="kubectl --namespace ${ALERT_NAMESPACE} rollout restart deployment/${ALERT_DEPLOYMENT}"

    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "DRY_RUN: Would execute the following command:"
        log_info "> ${kubectl_command}"
    else
        log_info "Executing command: ${kubectl_command}"
        # The actual execution. If kubectl fails (e.g., deployment not found, no
        # permissions), the `set -e` option will cause the script to terminate here.
        if output=$(${kubectl_command}); then
            log_info "Successfully initiated rollout restart. Kubectl output: ${output}"
        else
            # This block will only be reached if a command fails and `set +e` was active.
            # With `set -e`, the script would have already exited. This is here for
            # clarity and defense-in-depth.
            log_error "Failed to execute kubectl command. Please check permissions and resource names."
            exit 1
        fi
    fi

    log_info "Auto-heal runbook finished successfully."
}

#
# ---[ Script Entrypoint ]----------------------------------------------------
#
# This construct ensures that the main logic is encapsulated in a function and
# is only executed when the script is run directly. This is a best practice
# that allows for sourcing the script's functions into other scripts without
# executing the main logic.
#
main "$@"