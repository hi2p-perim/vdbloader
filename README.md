vdbloader
====================

This project provides a thin c-api wrapper for a part of features of OpenVDB required to support volume rendering in the renderer implementation. The main purpose of this project is to provide configuration-agnistic layer of OpenVDB in Windows environment.

## Build

This library is intended to used in conda environment and dependencies must be installed by conda accordingly. Once you install dependencies, you can build the library with CMake command.

```bash
$ conda install -c hi2p-perim openvdb
```

In Windows

```bash
$ cmake -H. -B_build -G "Visual Studio 15 2017 Win64" -D CMAKE_INSTALL_PREFIX=./dist
$ cmake --build _build --config Release --target install
```

