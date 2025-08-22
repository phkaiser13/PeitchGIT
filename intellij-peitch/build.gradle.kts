plugins {
    kotlin("jvm") version "2.0.0" apply false
    id("org.jetbrains.intellij") version "1.17.3" apply false
}

allprojects {
    group = "com.peitchgit"
    version = "0.1.0"

    repositories {
        mavenCentral()
    }
}
