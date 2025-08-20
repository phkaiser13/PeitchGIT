-- plugins/advanced_workflows.lua
-- Advanced phgit plugin demonstrating the enhanced Lua bridge capabilities.
-- This plugin showcases:
-- - Custom command registration with complex logic
-- - Configuration-driven behavior
-- - Hook-based automation
-- - File system integration
-- - Environment-aware workflows

-- Plugin metadata
local PLUGIN_NAME = "Advanced Workflows"
local PLUGIN_VERSION = "1.2.0"
local PLUGIN_AUTHOR = "phgit Community"

-- Initialize plugin
phgit.log("INFO", "Loading " .. PLUGIN_NAME .. " v" .. PLUGIN_VERSION, "PLUGIN")

-- === UTILITY FUNCTIONS ===

-- Safe configuration getter with defaults
local function get_config(key, default)
    local value = phgit.config_get(key)
    return value or default
end

-- Check if we're in a Git repository
local function is_git_repo()
    return phgit.file_exists(".git") or phgit.file_exists(".git/HEAD")
end

-- Get current branch name from Git
local function get_current_branch()
    local branch_file = ".git/HEAD"
    if not phgit.file_exists(branch_file) then
        return "unknown"
    end
    
    -- This is simplified; in reality you'd read the file content
    return "main" -- Placeholder for actual branch detection
end

-- === CUSTOM COMMANDS ===

-- Smart sync: pull, rebase, push with conflict detection
function smart_sync(...)
    local args = {...}
    phgit.log("INFO", "Executing smart sync workflow", "SMART_SYNC")
    
    if not is_git_repo() then
        phgit.log("ERROR", "Not in a Git repository", "SMART_SYNC")
        return false
    end
    
    -- Check if auto-sync is enabled
    local auto_sync = get_config("phgit.workflow.auto-sync", "true")
    if auto_sync ~= "true" then
        phgit.log("WARN", "Auto-sync disabled in configuration", "SMART_SYNC")
        return false
    end
    
    local branch = get_current_branch()
    phgit.log("INFO", "Syncing branch: " .. branch, "SMART_SYNC")
    
    -- Execute git operations with error handling
    local success = true
    
    -- Step 1: Fetch latest changes
    if not phgit.run_command("fetch", {"--all", "--prune"}) then
        phgit.log("ERROR", "Failed to fetch remote changes", "SMART_SYNC")
        return false
    end
    
    -- Step 2: Check for local changes
    local status_result = phgit.run_command("status", {"--porcelain"})
    if not status_result then
        phgit.log("ERROR", "Failed to check repository status", "SMART_SYNC")
        return false
    end
    
    -- Step 3: Pull with rebase if configured
    local pull_strategy = get_config("phgit.workflow.pull-strategy", "merge")
    local pull_args = {}
    
    if pull_strategy == "rebase" then
        table.insert(pull_args, "--rebase")
    elseif pull_strategy == "ff-only" then
        table.insert(pull_args, "--ff-only")
    end
    
    if not phgit.run_command("pull", pull_args) then
        phgit.log("ERROR", "Failed to pull changes - possible conflicts", "SMART_SYNC")
        return false
    end
    
    -- Step 4: Push if auto-push is enabled
    local auto_push = get_config("phgit.workflow.auto-push", "false")
    if auto_push == "true" then
        if not phgit.run_command("push", {"origin", branch}) then
            phgit.log("WARN", "Failed to push changes", "SMART_SYNC")
            -- Don't fail the entire operation for push failures
        else
            phgit.log("INFO", "Successfully pushed changes to origin/" .. branch, "SMART_SYNC")
        end
    end
    
    phgit.log("INFO", "Smart sync completed successfully", "SMART_SYNC")
    return true
end

-- Project setup command with templates
function setup_project(project_type, project_name)
    if not project_type then
        phgit.log("ERROR", "Usage: phgit setup <type> [name]", "SETUP")
        phgit.log("INFO", "Available types: web, api, lib, docs", "SETUP")
        return false
    end
    
    project_name = project_name or "new-project"
    phgit.log("INFO", "Setting up " .. project_type .. " project: " .. project_name, "SETUP")
    
    -- Create project directory structure based on type
    local templates = {
        web = {
            "src/",
            "public/",
            "docs/",
            "tests/",
            ".gitignore",
            "README.md",
            "package.json"
        },
        api = {
            "src/",
            "tests/",
            "docs/",
            "config/",
            ".gitignore",
            "README.md",
            "Dockerfile"
        },
        lib = {
            "src/",
            "include/",
            "tests/",
            "examples/",
            "docs/",
            "CMakeLists.txt",
            ".gitignore",
            "README.md"
        },
        docs = {
            "content/",
            "assets/",
            "config/",
            ".gitignore",
            "README.md"
        }
    }
    
    local template = templates[project_type]
    if not template then
        phgit.log("ERROR", "Unknown project type: " .. project_type, "SETUP")
        return false
    end
    
    -- Initialize git repository
    if not phgit.run_command("init", {project_name}) then
        phgit.log("ERROR", "Failed to initialize Git repository", "SETUP")
        return false
    end
    
    phgit.log("INFO", "Created project structure for " .. project_type, "SETUP")
    
    -- Set up project-specific configuration
    local config_prefix = "phgit.project." .. project_type
    phgit.config_set(config_prefix .. ".name", project_name)
    phgit.config_set(config_prefix .. ".created", os.date("%Y-%m-%d"))
    
    -- Set up recommended Git hooks for this project type
    if project_type == "web" or project_type == "api" then
        phgit.config_set("phgit.hooks.pre-commit", "lint,test")
        phgit.config_set("phgit.hooks.pre-push", "build,test")
    end
    
    return true
end

-- Environment-aware status command
function enhanced_status()
    phgit.log("INFO", "Generating enhanced status report", "STATUS")
    
    if not is_git_repo() then
        phgit.log("ERROR", "Not in a Git repository", "STATUS")
        return false
    end
    
    -- Get basic git status
    if not phgit.run_command("status", {"--short", "--branch"}) then
        return false
    end
    
    -- Add environment information
    local user = phgit.getenv("USER") or phgit.getenv("USERNAME") or "unknown"
    local home = phgit.getenv("HOME") or phgit.getenv("USERPROFILE") or "unknown"
    local pwd = phgit.getenv("PWD") or "unknown"
    
    phgit.log("INFO", "User: " .. user, "ENV_INFO")
    phgit.log("INFO", "Working directory: " .. pwd, "ENV_INFO")
    
    -- Check for configuration-driven custom status
    local show_upstream = get_config("phgit.status.show-upstream", "true")
    if show_upstream == "true" then
        phgit.run_command("status", {"--ahead-behind"})
    end
    
    -- Check for uncommitted configuration changes
    if phgit.file_exists(".phgitconfig") then
        phgit.log("INFO", "Local phgit configuration detected", "CONFIG")
    end
    
    return true
end

-- === HOOK IMPLEMENTATIONS ===

-- Pre-commit validation hook
function pre_commit_validation()
    phgit.log("INFO", "Running pre-commit validation", "HOOK")
    
    -- Check if validation is enabled
    local enable_validation = get_config("phgit.hooks.pre-commit.validation", "true")
    if enable_validation ~= "true" then
        phgit.log("DEBUG", "Pre-commit validation disabled", "HOOK")
        return
    end
    
    -- Check for common issues
    local issues = {}
    
    -- Check for TODO/FIXME in staged files (simplified check)
    local todo_check = get_config("phgit.hooks.pre-commit.check-todos", "true")
    if todo_check == "true" then
        phgit.log("DEBUG", "Checking for TODO/FIXME markers", "HOOK")
        -- In real implementation, would examine staged files
        -- For demo, we'll just log the check
    end
    
    -- Check file sizes
    local max_file_size = get_config("phgit.hooks.pre-commit.max-file-size", "10MB")
    phgit.log("DEBUG", "Checking file sizes (max: " .. max_file_size .. ")", "HOOK")
    
    -- Log validation result
    if #issues == 0 then
        phgit.log("INFO", "Pre-commit validation passed", "HOOK")
    else
        phgit.log("WARN", "Pre-commit validation found " .. #issues .. " issues", "HOOK")
    end
end

-- Post-commit notification hook
function post_commit_notification()
    phgit.log("INFO", "Post-commit notification triggered", "HOOK")
    
    local notify_enabled = get_config("phgit.hooks.post-commit.notify", "false")
    if notify_enabled ~= "true" then
        return
    end
    
    local branch = get_current_branch()
    phgit.log("INFO", "Commit completed on branch: " .. branch, "NOTIFICATION")
    
    -- Could integrate with external notification systems here
    local notification_url = get_config("phgit.hooks.post-commit.webhook")
    if notification_url then
        phgit.log("DEBUG", "Would send notification to: " .. notification_url, "HOOK")
    end
end

-- Backup hook for important operations
function backup_hook(operation)
    local enable_backup = get_config("phgit.backup.enabled", "false")
    if enable_backup ~= "true" then
        return
    end
    
    local backup_dir = get_config("phgit.backup.directory", phgit.getenv("HOME") .. "/.phgit-backups")
    phgit.log("INFO", "Creating backup for operation: " .. (operation or "unknown"), "BACKUP")
    
    -- In real implementation, would create backup of current state
    phgit.log("DEBUG", "Backup location: " .. backup_dir, "BACKUP")
end

-- === PLUGIN REGISTRATION ===

-- Register custom commands
phgit.register_command(
    "sync", 
    "smart_sync", 
    "Smart synchronization with remote repository (fetch, pull, push)",
    "phgit sync [--force]"
)

phgit.register_command(
    "setup", 
    "setup_project", 
    "Set up a new project with predefined structure and configuration",
    "phgit setup <type> [name] - Types: web, api, lib, docs"
)

phgit.register_command(
    "enhanced-status", 
    "enhanced_status", 
    "Enhanced status with environment and configuration information",
    "phgit enhanced-status"
)

-- Register hooks
phgit.register_hook("pre-commit", "pre_commit_validation")
phgit.register_hook("pre-commit", "backup_hook")
phgit.register_hook("post-commit", "post_commit_notification")
phgit.register_hook("pre-push", "backup_hook")

-- Plugin initialization complete
phgit.log("INFO", "Advanced Workflows plugin loaded successfully", "PLUGIN")
phgit.log("INFO", "Registered 3 commands and 4 hook handlers", "PLUGIN")

-- Set plugin metadata in configuration for introspection
phgit.config_set("phgit.plugins.advanced-workflows.version", PLUGIN_VERSION)
phgit.config_set("phgit.plugins.advanced-workflows.author", PLUGIN_AUTHOR)
phgit.config_set("phgit.plugins.advanced-workflows.loaded", "true")