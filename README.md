# Archon

The Archon project consists of a number of general purpose libraries:

  | Library | Description
  |---------|--------------------------------------------
  | Core    | Foundational and general purpose utilities
  | Log     | General purpose logging
  | CLI     | General purpose command line processing
  | Check   | General purpose testing framework
  | Math    | Vectors and matrices
  | Util    | Special purpose utilities
  | Image   | Load, save, and manipulate images
  | Font    | Font rendering
  | Display | OS GUI integration
  | Render  | Facilities for rendering of 3-D graphics


## Prerequisites for building

Here is the list of dependencies for the various libraries of the Archon project:

  | Name     | Minimum version | Archon library | Optional | Description
  |----------|-----------------|----------------|----------|-----------------------------------------------
  | `libpng` | 1.5.4           | Image          | Yes      | Image format (Portable Network Graphics)
  | FreeType |                 | Font           | Yes      | Font rendering facilities
  | Xlib     |                 | Display        | Yes      | X Window System protocol client library
  | SDL      | 2.0.22          | Display        | Yes      | OS GUI integration (Simple DirectMedia Layer)

See below for information on how to install these dependencies on various platforms.

### Ubuntu Linux

Run this command to install `libpng`, FreeType, Xlib, and SDL:

```sh
apt install libpng-dev libfreetype-dev libx11-dev libsdl2-dev
```

### macOS

Run this command to install `libpng`, FreeType, and SDL:

```sh
brew install libpng freetype sdl2
```

### Windows

Run this command to install `libpng`, FreeType, and SDL using [Vcpkg][vcpkg]:


```sh
c:\src\vcpkg\vcpkg.exe install libpng:x64-windows freetype:x64-windows sdl2:x64-windows
```

If Vcpkg in not already installed, you can install it by running these commands:

```sh
cd c:\src
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg.exe integrate install
```

[vcpkg]: https://github.com/Microsoft/vcpkg


## Build and run the test suite

The test suite can be built and run through use of CTest (part of CMake). Below are
alternative ways of doing it, which offer more control over the testing process.

### Linux, macOS, and other UNIX-like systems

Enter the root directory of the checked out Archon project, then run this command to build
and run the test suite:

```sh
bash do.sh check-debug
```

The `check-debug` function of `do.sh` offers a number of useful command line options. To see
them, add `--help` to the command shown above.

The `do.sh` shell script offers a number of other functions beyond `check-debug`. To see a
list of available function, run `bash do.sh help`.

### Windows

1. Launch Visual Studio.
2. Chose "Open a local folder".
3. Select the folder holding the checked out Archon project.
4. Wait for Visual Studio to finish CMake generation.
5. From the "Test" menu, chose "Run CTests for Archon".

Alternatively, after CMake generation has finished:
1. Open `src/archon/test.cpp` inside Visual Studio, and make it the active window.
2. From the "Debug" menu, chose "Start Without Debugging".

Alternatively, if [Git for Windows][gitfw] is installed, open Git Bash and enter the root
directory of the checked out Archon project, then run:

```sh
bash do.sh check-debug
```

See above for additional possibilities when using `do.sh`.

[gitfw]: Git for Windows can be installed from [https://gitforwindows.org/](https://gitforwindows.org/).
