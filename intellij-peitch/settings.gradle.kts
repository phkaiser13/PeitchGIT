rootProject.name = "intellij-peitch"

include(
    ":peitchgit-core",
    ":ui-components",
    ":platform-integration",
    ":feature-modules:github-integration",
    ":feature-modules:cicd-pipelines",
    ":feature-modules:lua-scripting"
)
