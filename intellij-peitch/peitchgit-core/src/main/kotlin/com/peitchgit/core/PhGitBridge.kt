package com.peitchgit.core

// Bridge for communicating with PhGit via CLI or JNI.
// For now, a thin CLI wrapper is suggested (spawn processes, parse JSON).
class PhGitBridge {
    fun version(): String = "0.1.0"
    fun run(args: List<String>): Int {
        // TODO: execute phgit binary and return exit code
        return 0
    }
}
