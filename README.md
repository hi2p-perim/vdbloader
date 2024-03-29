vdbloader
====================

This project provides a thin c-api wrapper for a part of features of OpenVDB required to support volume rendering in the renderer implementation. The main purpose of this project is to provide configuration-agnostic layer of OpenVDB in Visual Studio environment.

## Build

This library is intended to be used in conda environment and dependencies must be installed by conda install command accordingly. Once you install dependencies, you can build the library with CMake command.

```bash
$ conda install -c hi2p-perim openvdb
```

In Windows

```bash
$ cmake -H. -B_build -G "Visual Studio 15 2017 Win64" -D CMAKE_INSTALL_PREFIX=./dist
$ cmake --build _build --config Release --target install
```

In Linux

```bash
$ cmake -H. -B_build -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=./dist
$ cmake --build _build --target install
```