plugins {
    kotlin("jvm") version "2.0.0"
    id("org.jetbrains.intellij") version "1.17.3"
}

repositories {
    mavenCentral()
}

intellij {
    version.set("2024.3")
    plugins.set(listOf("com.intellij.java"))
}

dependencies {
    // Example: implementation(project(":peitchgit-core"))
}

kotlin {
    jvmToolchain(17)
}
