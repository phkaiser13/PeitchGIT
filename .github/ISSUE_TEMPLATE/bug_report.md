---
name: "Bug report"
about: "Create a clear, actionable report to help us debug and fix issues in gitph."
title: "[BUG] "
labels: ["bug", "triage"]
assignees: []
---

# Bug Report

**Summary**
A one-line summary of the bug.

---

## Steps to reproduce
Please provide the minimal, precise steps required to reproduce the issue.  
1. Step one (e.g., open repository `X`)  
2. Run: `gitph <command> <args>`  
3. Observe the error

> If possible, include a minimal repository, a small script, or exact commands that reproduce the problem.

---

## Expected behavior
Describe what you expected to happen.

---

## Actual behavior
Describe what actually happened, including error messages, stack traces, and exit codes. Paste full output inside a fenced code block:

```

\<copy-paste error / stack trace here>

```

---

## Reproduction example / Minimal test case
If applicable, include a short reproducible example (commands, small code snippet, repository link).

---

## Environment
Please paste output or fill in the fields below:

- **OS:** (e.g., `Ubuntu 22.04`, `Windows 11`, `macOS 14.5`)  
- **gitph version:** (e.g., `gitph --version` output)  
- **Installed from:** (e.g., Homebrew, apt, pip, source, release tarball)  
- **Shell / Terminal:** (e.g., `bash`, `zsh`, `PowerShell`)  
- **Architecture:** (e.g., `x86_64`, `arm64`)

Helpful commands you can paste here:
```

gitph --version
uname -a

```

---

## Logs / Debug output
If available, run the command with debug logging and paste the output (or attach logs as files). Example:
```

gitph <command> --debug

```

---

## Screenshots / GIFs (optional)
Attach screenshots or a short GIF showing the issue.

---

## Severity / Impact
Select one (helps prioritization):
- [ ] Blocker — prevents using gitph entirely
- [ ] Major — important functionality broken / no reasonable workaround
- [ ] Minor — partial loss of functionality; workaround exists
- [ ] Trivial — cosmetic or very small issue

---

## Workarounds (if any)
Describe any temporary fixes or steps that mitigate the issue.

---

## Additional context
Add any other context, related issues, or notes (e.g., when it started, whether it affects multiple machines/users).

---

**Security / Sensitive data note:** please redact any secrets, API tokens, credentials, or personal data. If logs contain sensitive information, attach a sanitized file instead.
