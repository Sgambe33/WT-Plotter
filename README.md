# Description

This is a template project to create a modularized JavaFX application. It uses Gradle for its build and includes
[The Badass JLink Plugin](https://badass-jlink-plugin.beryx.org/releases/latest/) to create custom runtime images 
and platform specific applications with bundled runtimes with jpackage.

## Logging

It includes Logback for logging and it comes with a reasonable default logging configuration that logs to STDOUT and 
to a file at the path `${user.home}/app.log`

## Modularization

The application is modularized so that slimmed down runtimes can be bundled with the application.

# Using the Project

## Git

1. Clone the project with `git clone https://github.com/mjparme/javafx-template.git <name-of-your-project>` (or fork it and create a clone of your fork)
1. Remove the `.git` directory so it is no longer tied to this repo 
1. Execute `git init .` to put it back under version control
1. Push to a remote if you want 

## IntelliJ

IntelliJ will happily configure itself from a Gradle build. 

1. Open IntelliJ
2. File -> Open
3. In the file chooser dialog select the directory that contains the local repository and press the `Open` button. 
IntelliJ will notice the directory has a `build.gradle` in it and then do its thing and in a few minutes you should be good to go.
4. To run it go to `MyApplication.java` and press the green arrow in the gutter next to the class name or 
next to the `main()` method. Or you can open a terminal and execute `./gradlew run`


# Gradle

The project includes the [Gradle Wrapper](https://docs.gradle.org/current/userguide/gradle_wrapper.html) so no local installation of Gradle is needed. 

## Tasks

* `./gradlew run` -- Pretty self explantory, will run the application. 
* `./gradlew jpackage` -- Will package your app with a bundled runtime in a platform specific format. After this runs check the `build/jpacakge` directory for the bundled runtimes. On a Mac this will be a `.dmg`, `.pkg`, and `.app` file. On Windows it will be an `.exe` and `.msi` I think, and a `.rpm` file on linux I think. I have only ever ran this on a Mac so on different platforms just run it and see what you get.  
