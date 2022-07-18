Project64 - Building from source
================================

This document describes how to set up a local build environment for Project64 on Windows.
It is meant for helping contributors set up building Project64 on their machines so that they can write code to fix bugs and add new features.
If you just want to run Project64 or its development builds, use the [Readme](https://github.com/project64/project64/blob/develop/README.md) instead.

## Required software

* Git
    * This is a hard requirement. It is used for the build step and requires the solution be a part of the Project64 git repository on disk.
* Visual Studio 2015, 2017, or 2019 Community Edition
* During installation, select the `Programming Languages/Visual C++` option in the Visual Studio 2015 installer, or
* During installation, select the `Desktop development with C++` workload in Visual Studio 2017 and 2019

## Clone the repository

You must clone the repository to be able to build many of the projects in the solution.
Use the following command to clone the latest repository code

```
git clone https://github.com/project64/project64.git
```

## Build from source

Open the `Project64.sln` file in Visual Studio. You can now build the solution from the Build menu.

Building a Release build will also generate an installer file alongside the output binary that you can use to test the installation process.

In the current state of Project64 (March 2021) you will get errors from trying to build the Android projects. These error messages don't mean anything for the Windows builds. You can safely unload the offending projects from the Solution View to reduce clutter in the Build Log if you are only planning to contribute to the Windows builds.

## Recommended additional steps

* If you wish to quickly launch the Project64 application with Visual Studio's debugger you should right-click the Project64 project in the Solution View and choose "Set as Startup Project" in the context menu. Pressing F5 or the Local Windows Debugger option should now launch the Project64 application.

* In the `Config` folder in the root of the repository is a `Project64.cfg.development` file. Copying this file over top of the `Project64.cfg` file in the same directory will ensure the builds in the `Bin` subdirectories have the proper directories set for accurate debugging.
