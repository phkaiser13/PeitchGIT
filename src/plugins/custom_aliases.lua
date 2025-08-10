-- Copyright (C) 2025 Pedro Henrique / phkaiser13
-- custom_aliases.lua - Example plugin for creating custom command aliases.
--
-- This script demonstrates how a user can extend gitph with their own
-- command shortcuts. It uses a high-level API function to register
-- new commands that map to existing ones.
--
-- This illustrates the flexibility of the scripting engine: tailoring
-- the tool to fit the user's personal workflow.
--
-- SPDX-License-Identifier: GPL-3.0-or-later

-- Log to the gitph log file to confirm the plugin was loaded.
gitph.log("INFO", "custom_aliases.lua plugin loaded.")

-- Utility function to safely register an alias if the API exists.
local function safe_register_alias(alias, command)
    if gitph.register_alias then
        local success, err = pcall(gitph.register_alias, alias, command)
        if success then
            gitph.log("INFO", string.format("Registered alias '%s' for command '%s'.", alias, command))
        else
            gitph.log("ERROR", string.format("Failed to register alias '%s': %s", alias, err or "Unknown error"))
        end
    else
        gitph.log("WARN", "gitph.register_alias function not found. Cannot create aliases.")
        gitph.log("WARN", "This may require a newer version of the gitph core.")
    end
end

-- Register custom aliases.
safe_register_alias("st", "status") -- 'st' will execute 'status'
safe_register_alias("snd", "SND")   -- 'snd' will execute 'SND'
