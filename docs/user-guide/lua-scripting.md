# Extending phgit with Lua

One of `phgit`'s most powerful features is its extensibility through a built-in Lua scripting engine. This allows you to tailor `phgit` to your specific workflow without modifying the core application. You can create custom command aliases, hook into Git events to enforce policies, and much more.

## The Plugin System

`phgit` automatically loads any Lua script (`.lua` file) it finds in the `src/plugins/` directory at startup. To create a new plugin, simply add a new `.lua` file to this directory.

## The `phgit` Global Table

When your script is executed, `phgit` exposes a global Lua table named `phgit`. This table contains all the API functions you can use to interact with the core application.

### Logging from Lua

You can write to the main `phgit` log file from your script using the `phgit.log()` function. This is useful for debugging your plugins.

**Syntax:**
`phgit.log(log_level, message)`

- `log_level`: A string representing the level ("INFO", "WARN", "ERROR", "DEBUG").
- `message`: The string message to log.

**Example:**
```lua
phgit.log("INFO", "My custom plugin has loaded successfully!")
```

## Creating Command Aliases

Do you have a long `phgit` command you type frequently? You can create a shorter alias for it with the `phgit.register_alias()` function.

**Syntax:**
`phgit.register_alias(alias, original_command)`

- `alias`: The new, shorter command name.
- `original_command`: The existing `phgit` command you want to alias.

**Example: `my_aliases.lua`**
Let's create a plugin to add some convenient shortcuts. Create a file named `src/plugins/my_aliases.lua`:

```lua
-- File: src/plugins/my_aliases.lua

-- Log to confirm the plugin is loading
phgit.log("INFO", "Loading custom aliases.")

-- Create a short alias 'st' for the 'status' command
phgit.register_alias("st", "status")

-- Create a 'commit-push' alias for the 'SND' command
phgit.register_alias("cp", "SND")

phgit.log("INFO", "Custom aliases 'st' and 'cp' registered.")
```

After restarting `phgit` (or running it for the first time after adding the file), you can now run `phgit st` instead of `phgit status`, and `phgit cp` instead of `phgit SND`.

## Hooking into the Git Lifecycle

`phgit` allows you to define special global functions in your Lua scripts that act as "hooks." These hooks are automatically called by the `phgit` core at specific points in the Git lifecycle, allowing you to run custom logic.

### The `on_pre_push` Hook

The `on_pre_push` hook is called just before a `git push` operation is executed (for example, via the `SND` command). It allows you to inspect the push and even cancel it if certain conditions aren't met.

**Syntax:**
`function on_pre_push(remote, branch)`

- `remote`: The name of the remote the push is targeting (e.g., "origin").
- `branch`: The name of the branch being pushed.
- **Return Value**:
  - `true`: The push is allowed to proceed.
  - `false`: The push is cancelled.

**Example: Enforcing Branch Policy**
Let's create a plugin to prevent direct pushes to the `main` or `master` branches. Create a file named `src/plugins/policy.lua`:

```lua
-- File: src/plugins/policy.lua

-- This global function is automatically called by the phgit core before a push.
function on_pre_push(remote, branch)
    if branch == "main" or branch == "master" then
        -- Print a message to the user
        print("[POLICY] Direct pushes to the '" .. branch .. "' branch are forbidden.")
        print("Please use a pull request workflow.")

        -- Cancel the push
        return false
    end

    -- Allow pushes to any other branch
    return true
end
```

With this plugin active, any attempt to push to `main` or `master` using a `phgit` command will be blocked, helping you enforce team development policies.
