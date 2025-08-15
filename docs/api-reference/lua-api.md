# Lua API Reference

This document provides a technical reference for the API exposed to Lua scripts by the `gitph` core.

## The `gitph` Table

All core functions are exposed through a single global table named `gitph`.

---

### `gitph.log(level, message)`

Writes a message to the main `gitph` log file.

- **Parameters:**
  - `level` (string): The log level. Must be one of `"DEBUG"`, `"INFO"`, `"WARN"`, `"ERROR"`, or `"FATAL"`.
  - `message` (string): The log message to write.

- **Returns:** `nil`

- **Example:**
  ```lua
  gitph.log("INFO", "My plugin has started.")
  ```

---

### `gitph.register_alias(alias, original_command)`

Creates a new command that serves as an alias for an existing command. This must be called during the initial script load.

- **Parameters:**
  - `alias` (string): The new, shorter command name you want to create.
  - `original_command` (string): The name of the existing `gitph` command that the alias should execute.

- **Returns:** `nil`

- **Example:**
  ```lua
  -- Allows 'gitph st' to run the 'status' command.
  gitph.register_alias("st", "status")
  ```

---

## Global Hook Functions

`gitph` can be configured to call specific, globally-defined functions in your Lua scripts at certain points in its lifecycle. You implement a hook by simply defining a global function with the correct name and parameters.

---

### `on_pre_push(remote, branch)`

This function, if defined, is called just before a `git push` operation is executed by a `gitph` command (e.g., `SND`). It allows you to cancel the push.

- **Parameters:**
  - `remote` (string): The name of the remote target (e.g., `"origin"`).
  - `branch` (string): The name of the branch being pushed (e.g., `"feature/my-work"`).

- **Return Value:**
  - `boolean`:
    - `true`: The push is allowed to continue.
    - `false` or `nil`: The push is aborted.

- **Example:**
  ```lua
  function on_pre_push(remote, branch)
    if branch == "main" then
      print("Error: Direct pushes to main are not allowed.")
      return false
    end
    return true
  end
  ```
