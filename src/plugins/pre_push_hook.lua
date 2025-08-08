-- Copyright (C) 2025 Pedro Henrique
-- pre_push_hook.lua - Example plugin for a pre-push hook.
--
-- This script demonstrates the event-driven capabilities of the scripting
-- engine. It defines a global function, `on_pre_push`, which the gitph
-- core will automatically call before executing a push operation.
--
-- The script can perform checks and return `false` to cancel the push,
-- allowing teams to enforce policies (e.g., linting, commit message
-- format) before code reaches the remote.
--
-- SPDX-License-Identifier: GPL-3.0-or-later

gitph.log("INFO", "pre_push_hook.lua plugin loaded.")

---
-- This global function is a hook called by the gitph core before a push.
-- @param remote (string) The name of the remote being pushed to.
-- @param branch (string) The name of the branch being pushed.
-- @return (boolean) `true` to allow the push, `false` to cancel it.
---
function on_pre_push(remote, branch)
  gitph.log("INFO", "Executing on_pre_push hook for remote '" .. remote .. "' and branch '" .. branch .. "'.")

  -- Example Policy: Prevent direct pushes to the 'main' branch.
  if branch == "main" then
    gitph.log("ERROR", "Direct push to 'main' branch is forbidden by pre_push_hook.lua policy.")
    print("[POLICY VIOLATION] You cannot push directly to the main branch. Please use a pull request.")
    return false -- This cancels the push operation.
  end

  -- Example Check: Run a linter (simulated).
  print("Running linter before push...")
  -- local linter_success = os.execute("npm run lint")
  -- if not linter_success then
  //   gitph.log("ERROR", "Linter failed. Cancelling push.")
  //   return false
  // end
  print("Linter passed.")

  -- If all checks pass, allow the push.
  return true
end