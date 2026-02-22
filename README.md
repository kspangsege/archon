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
  | GFX     | Graphics utilities
  | Display | OS GUI integration
  | Render  | Facilities for rendering of 3-D graphics


## Prerequisites for building

Here is the list of dependencies for the various libraries of the Archon project:

  | Name      | Minimum version | Archon library | Optional | Description
  |-----------|-----------------|----------------|----------|-----------------------------------------------
  | `libpng`  | 1.5.4           | Image          | Yes      | PNG image file format
  | `libjpeg` | 6b              | Image          | Yes      | JPEG image file format
  | FreeType  | 2.10            | Font           | Yes      | Font rendering facilities
  | Xlib      | 1.3.0           | Display        | Yes      | X Window System protocol client library
  | GLX       |                 | Display        | Yes      | OpenGL Extension to the X Window System
  | SDL       | 3.2.20          | Display        | Yes      | OS GUI integration (Simple DirectMedia Layer)
  | OpenGL    |                 | Display        | Yes      | Open Graphics Library
  | GLEW      |                 | Display        | Yes      | The OpenGL Extension Wrangler Library

Note: `libjpeg-turbo` can be used in place of `libjpeg`.

See below for information on how to install these dependencies on various platforms.

### Ubuntu Linux

Run this command to install `libpng`, `libjpeg`, FreeType, Xlib, GLX, OpenGL, and GLEW:

```sh
apt install libpng-dev libjpeg-dev libfreetype-dev libx11-dev libxext-dev libxrandr-dev libxrender-dev libglx-dev libgl-dev libglew-dev
```

Since Ubuntu 25.10, it is possible install SDL 3 using:

```sh
apt install libsdl3-dev
```

On Ubuntu 24.04, one can install SDL 3 by building it from source. Check out the SDL
source code from GitHub using:

```sh
git clone git@github.com:libsdl-org/SDL.git
```

Install build dependencies using:

```sh
apt install libxinerama-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev libwayland-dev wayland-protocols libdecor-0-dev libegl-dev libdrm-dev libgbm-dev libvulkan-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libsndio-dev libusb-1.0-0-dev libudev-dev libxkbcommon-dev libibus-1.0-dev
```

Finally, to configure, build, and install SDL, enter the directory in which SDL was checked
out, then run:

```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSDL_X11_XTEST=OFF
make -j
make install
```


### macOS

Run this command to install `libpng`, `libjpeg`, FreeType, SDL, and GLEW using
[Homebrew][homebrew]:

```sh
brew install libpng libjpeg freetype sdl3 glew
```

### Windows

Run this command to install `libpng`, `libjpeg-turbo`, FreeType, SDL, and GLEW using
[Vcpkg][vcpkg]:


```sh
vcpkg install --triplet x64-windows libpng libjpeg-turbo freetype sdl3 glew
```

This assumes that Vcpkg is installed and can be found via the `PATH` environment variable.

If Vcpkg in not already installed, you can install it in your home directory by running
these commands from a command prompt[^wincmdprompt]:

```sh
git clone https://github.com/Microsoft/vcpkg.git %USERPROFILE%\vcpkg
%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat
```

To allow for Vcpkg to be found from the command line, add `%USERPROFILE%\vcpkg` to the
`PATH` environment variable using the system tool, "Edit the system environment variables".

If you want to build the Archon project from within the Visual Studio UI, tell Visual Studio
about your Vcpkg installation by running this command:

```sh
vcpkg integrate install
```


[homebrew]: https://brew.sh/
[vcpkg]: https://github.com/Microsoft/vcpkg

[^wincmdprompt]: You can use the regular system Command Prompt.


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

Alternatively, if Git for Windows[^gitfw] is installed, open Git Bash and enter the root
directory of the checked out Archon project, then run:

```sh
bash do.sh check-debug
```

The `do.sh` script assumes that the dependencies were installed using Vcpkg, and that the
`vcpkg` command can be found via the `PATH` environment variable. It also assumes that the
`cmake` command can be found via the `PATH` environment variable. To use the CMake
installation that is part of Visual Studio, add `<cmake root>\bin` to the `PATH` environment
variable using the system tool, "Edit the system environment variables". For the community
edition of Visual Studio 2022, `<cmake path>` might be
`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake`.
For the community edition of Visual Studio 2026, it might instead be
`C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake`.

See above for additional possibilities when using `do.sh`.

[^gitfw]: Git for Windows can be installed from https://git-scm.com/download/win. See also
    https://gitforwindows.org/.
