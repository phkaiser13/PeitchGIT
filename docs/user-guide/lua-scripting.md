# Extending gitph with Lua

One of `gitph`'s most powerful features is its extensibility through a built-in Lua scripting engine. This allows you to tailor `gitph` to your specific workflow without modifying the core application. You can create custom command aliases, hook into Git events to enforce policies, and much more.

## The Plugin System

`gitph` automatically loads any Lua script (`.lua` file) it finds in the `src/plugins/` directory at startup. To create a new plugin, simply add a new `.lua` file to this directory.

## The `gitph` Global Table

When your script is executed, `gitph` exposes a global Lua table named `gitph`. This table contains all the API functions you can use to interact with the core application.

### Logging from Lua

You can write to the main `gitph` log file from your script using the `gitph.log()` function. This is useful for debugging your plugins.

**Syntax:**
`gitph.log(log_level, message)`

- `log_level`: A string representing the level ("INFO", "WARN", "ERROR", "DEBUG").
- `message`: The string message to log.

**Example:**
```lua
gitph.log("INFO", "My custom plugin has loaded successfully!")
```

## Creating Command Aliases

Do you have a long `gitph` command you type frequently? You can create a shorter alias for it with the `gitph.register_alias()` function.

**Syntax:**
`gitph.register_alias(alias, original_command)`

- `alias`: The new, shorter command name.
- `original_command`: The existing `gitph` command you want to alias.

**Example: `my_aliases.lua`**
Let's create a plugin to add some convenient shortcuts. Create a file named `src/plugins/my_aliases.lua`:

```lua
-- File: src/plugins/my_aliases.lua

-- Log to confirm the plugin is loading
gitph.log("INFO", "Loading custom aliases.")

-- Create a short alias 'st' for the 'status' command
gitph.register_alias("st", "status")

-- Create a 'commit-push' alias for the 'SND' command
gitph.register_alias("cp", "SND")

gitph.log("INFO", "Custom aliases 'st' and 'cp' registered.")
```

After restarting `gitph` (or running it for the first time after adding the file), you can now run `gitph st` instead of `gitph status`, and `gitph cp` instead of `gitph SND`.

## Hooking into the Git Lifecycle

`gitph` allows you to define special global functions in your Lua scripts that act as "hooks." These hooks are automatically called by the `gitph` core at specific points in the Git lifecycle, allowing you to run custom logic.

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

-- This global function is automatically called by the gitph core before a push.
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

With this plugin active, any attempt to push to `main` or `master` using a `gitph` command will be blocked, helping you enforce team development policies.
