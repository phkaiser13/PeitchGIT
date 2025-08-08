-- Copyright (C) 2025 Pedro Henrique
-- custom_aliases.lua - Example plugin for creating custom command aliases.
--
-- This script demonstrates how a user can extend gitph with their own
-- command shortcuts. It uses a (proposed) high-level API function to
-- register a new command that simply calls an existing one.
--
-- This illustrates the core value of the scripting engine: tailoring the
-- tool to the user's personal workflow.
--
-- SPDX-License-Identifier: GPL-3.0-or-later

-- Log to the gitph log file to confirm the plugin was loaded.
gitph.log("INFO", "custom_aliases.lua plugin loaded.")

-- Check if the gitph.register_alias function exists.
-- This is good practice for writing robust plugins.
if gitph.register_alias then

  -- Register 'st' as an alias for the 'status' command.
  -- The core would handle making this new 'st' command appear in the
  -- TUI menu and be dispatchable by the CLI.
  gitph.register_alias("st", "status")
  gitph.log("INFO", "Registered 'st' as an alias for 'status'.")

  -- Register 'snd' as a case-insensitive alias for 'SND'.
  gitph.register_alias("snd", "SND")
  gitph.log("INFO", "Registered 'snd' as an alias for 'SND'.")

else
  gitph.log("WARN", "gitph.register_alias not found. Cannot create aliases.")
  gitph.log("WARN", "This may require a newer version of the gitph core.")
end