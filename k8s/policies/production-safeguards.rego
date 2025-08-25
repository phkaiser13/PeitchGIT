#/*
# * Copyright (C) 2025 Pedro Henrique / phkaiser13
# *
# * File: k8s/policies/production-safeguards.rego
# *
# * This Rego policy file defines a comprehensive set of admission control rules for a Kubernetes
# * environment, enforced via a policy engine like OPA Gatekeeper. Its purpose is to establish a
# * strong security and stability baseline for all workloads deployed to a production cluster.
# * By codifying best practices, it prevents common misconfigurations that can lead to security
# * vulnerabilities, application downtime, and inefficient resource utilization.
# *
# * The architecture of this policy is a collection of individual 'deny' rules. Each rule
# * checks for a specific violation. If a submitted Kubernetes resource manifest violates any
#
# * of these rules, the admission request is rejected with a clear, descriptive message
# * explaining the violation and how to fix it. This proactive approach shifts security and
# * operational excellence "left," catching issues before they are deployed.
# *
# * This file consolidates multiple essential policies:
# * 1.  **Disallow 'latest' Tag:** Prevents the use of ambiguous 'latest' image tags, ensuring
# * deployment immutability and predictability.
# * 2.  **Require Liveness Probes:** Enforces the definition of liveness probes, allowing
# * Kubernetes to automatically restart unhealthy containers.
# * 3.  **Require Readiness Probes:** Mandates readiness probes to ensure traffic is only
# * routed to containers that are ready to serve.
# * 4.  **Disallow Privileged Containers:** Blocks containers from running in privileged mode,
# * significantly reducing the cluster's attack surface.
# * 5.  **Require Resource Limits:** Ensures that all containers have explicit CPU and memory
# * limits, preventing resource contention and noisy neighbor problems.
# * 6.  **Require Key Labels:** Mandates the presence of 'owner' and 'app' labels for proper
# * resource tracking, management, and cost allocation.
# *
# * This policy serves as a critical infrastructure component, acting as a gatekeeper to
# * maintain the health, security, and integrity of the Kubernetes cluster.
# *
# * SPDX-License-Identifier: Apache-2.0
# */

package kubernetes.policies

#
# ---[ Helper Functions ]-----------------------------------------------------
#

# Utility function to check if a container is present in the resource spec.
# This helps avoid errors when evaluating resources that don't have containers,
# like Services or ConfigMaps.
has_containers {
	input.request.kind.kind == "Pod"
	count(input.request.object.spec.containers) > 0
}

has_containers {
	input.request.kind.kind == "Deployment"
	count(input.request.object.spec.template.spec.containers) > 0
}

has_containers {
	input.request.kind.kind == "StatefulSet"
	count(input.request.object.spec.template.spec.containers) > 0
}

has_containers {
	input.request.kind.kind == "DaemonSet"
	count(input.request.object.spec.template.spec.containers) > 0
}

#
# ---[ Policy: Image Tag Best Practices ]-------------------------------------
#
# Deny deployments that use the ':latest' image tag.
#
# **Reasoning:** Using the ':latest' tag is considered an anti-pattern in production
# environments. It makes deployments unpredictable, as the actual image behind the tag
# can change at any time without a corresponding change in the manifest. This can lead
# to unexpected behavior, difficult rollbacks, and breaks the principle of immutability.
# Deployments must use specific, immutable image tags (e.g., version numbers, commit SHAs)
# to ensure that the exact same code is running every time a manifest is applied.
#
deny[msg] {
	has_containers
	container := input.request.object.spec.template.spec.containers[_]
	endswith(container.image, ":latest")
	msg := sprintf("Image '%v' uses the mutable ':latest' tag. Please use a specific, immutable tag.", [container.image])
}

#
# ---[ Policy: Health Probes ]------------------------------------------------
#
# Deny containers that do not have a liveness probe defined.
#
# **Reasoning:** A liveness probe is crucial for application reliability. It allows
# Kubernetes' kubelet to periodically check if a container is still running correctly.
# If the probe fails, Kubernetes will automatically restart the container, helping to
# recover from deadlocks or other unrecoverable states without manual intervention.
#
deny[msg] {
	has_containers
	container := input.request.object.spec.template.spec.containers[_]
	not container.livenessProbe
	msg := sprintf("Container '%v' must have a livenessProbe defined.", [container.name])
}

# Deny containers that do not have a readiness probe defined.
#
# **Reasoning:** A readiness probe indicates whether an application is ready to start
# serving traffic. Without it, Kubernetes might route traffic to a container that is
# still starting up, hasn't loaded its configuration, or can't connect to its
# backends. This would result in user-facing errors. A readiness probe ensures
# that a container is only added to a Service's load balancing pool when it is
# fully prepared to handle requests.
#
deny[msg] {
	has_containers
	container := input.request.object.spec.template.spec.containers[_]
	not container.readinessProbe
	msg := sprintf("Container '%v' must have a readinessProbe defined.", [container.name])
}

#
# ---[ Policy: Security Context ]---------------------------------------------
#
# Deny containers running in privileged mode.
#
# **Reasoning:** A privileged container (`securityContext.privileged: true`) breaks
# out of the container isolation and gains nearly all the capabilities of its host
# machine. This is an extreme security risk. A compromised privileged container could
# lead to a full cluster compromise. This capability should be disabled by default
# and only be granted in very rare, highly scrutinized circumstances.
#
deny[msg] {
	has_containers
	container := input.request.object.spec.template.spec.containers[_]
	container.securityContext.privileged
	msg := sprintf("Privileged container '%v' is not allowed. This poses a significant security risk.", [container.name])
}

#
# ---[ Policy: Resource Management ]------------------------------------------
#
# Deny containers that do not specify CPU and Memory limits.
#
# **Reasoning:** Setting resource limits (CPU and memory) is fundamental to ensuring
# cluster stability and fair resource sharing among tenants. Without limits, a single
# runaway container could consume all available CPU or memory on a node, causing
# performance degradation or outages for all other applications on that node (a "noisy
# neighbor" problem). Enforcing limits is a cornerstone of production-readiness.
#
deny[msg] {
	has_containers
	container := input.request.object.spec.template.spec.containers[_]
	not container.resources.limits.cpu
	msg := sprintf("Container '%v' must have CPU limits defined.", [container.name])
}

deny[msg] {
	has_containers
	container := input.request.object.spec.template.spec.containers[_]
	not container.resources.limits.memory
	msg := sprintf("Container '%v' must have memory limits defined.", [container.name])
}

#
# ---[ Policy: Resource Metadata and Labeling ]-------------------------------
#
# Deny resources that are missing key identifying labels ('owner', 'app').
#
# **Reasoning:** Consistent labeling is essential for managing a Kubernetes cluster
# at scale. Labels are used to organize resources, apply network policies, perform
# targeted operations (e.g., 'kubectl get pods -l app=backend'), and for cost
# allocation and chargeback. Requiring 'owner' and 'app' labels ensures that all
# resources can be easily identified, managed, and audited.
#
deny[msg] {
	not input.request.object.metadata.labels.owner
	msg := "Resource must have an 'owner' label."
}

deny[msg] {
	not input.request.object.metadata.labels.app
	msg := "Resource must have an 'app' label."
}