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


## Dependencies

Here is the list of dependencies for the various libraries of the Archon project:

  | Name                | Minimum version | Archon library | Optional | Description
  |---------------------|-----------------|----------------|----------|-----------------------------------------------
  | `libpng`            | 1.5.4           | Image          | Yes      | PNG image file format
  | `libjpeg`[^ljturbo] | 6b              | Image          | Yes      | JPEG image file format
  | FreeType            | 2.10            | Font           | Yes      | Font rendering facilities
  | Xlib                | 1.3.0           | Display        | Yes      | X Window System protocol client library
  | GLX                 |                 | Display        | Yes      | OpenGL Extension to the X Window System
  | SDL                 | 3.2.20          | Display        | Yes      | OS GUI integration (Simple DirectMedia Layer)
  | OpenGL              |                 | Display        | Yes      | Open Graphics Library
  | GLEW                |                 | Display        | Yes      | The OpenGL Extension Wrangler Library

See [below](#installing-dependencies) for information on how to install these dependencies
on various systems.

> **Important Note on Dynamic Linking & ABI**
>
> When a dependency, such as FreeType, is linked dynamically at build time, the dynamic
> library (`.so`, `.dylib`, `.dll`) that is provided at runtime must be at least the same
> version as was used at build time. The Archon source code relies on this. It assumes that
> dependency versions specified in headers, such as `FREETYPE_MAJOR` and `FREETYPE_MINOR`,
> can be trusted as specifying the minimum version of the dependency that can occur at
> runtime.
>
> Behavior is undefined (bad things can happen) if the version of a dynamically linked
> dependency provided at runtime is lower than what was used at build time. Note that this
> restriction is standard practice for open-source C++ projects.

[^ljturbo]: `libjpeg-turbo` can be used in place of `libjpeg`.


## Building and Installing

Below are the standard CMake instructions for configuring, building, and installing the
Archon libraries. See [further down](#convenience-shell-script) for information on using the
convenience shell script (`do.sh`). See also the section on [Building and Testing under
Visual Studio](#building-and-testing-under-visual-studio).

1. **Configure the project:**

   ```sh
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```

   *(Windows Vcpkg users: You will need to pass your toolchain file to CMake so it can find
   the dependencies: `-DCMAKE_TOOLCHAIN_FILE=[path to
   vcpkg]/scripts/buildsystems/vcpkg.cmake`)*

2. **Build the libraries:**

   ```sh
   cmake --build build -j
   ```

3. **Install the libraries:**

   ```sh
   cmake --install build
   ```

   *(Prepend `sudo` if you need to)*


## Testing

The test suite for the Archon libraries can be executed using CTest, which is part of
CMake. After [building](#building-and-installing), execute the test suite as follows:

```sh
ctest --test-dir build --output-on-failure
```

The test suite can also be executed through use of the [convenience shell script
(`do.sh`)](#convenience-shell-script). This offers finer control over the testing process
than is available when using `ctest`. The convenience shell script just calls the `test`
command (`build/src/test`), which is built alongside the libraries, so the same control is
available when directly executing the `test` command.

There is also a suite of demo programs that are built alongside the libraries. See [Demo
Programs](#demo-programs) below.


## Building and Testing under Visual Studio

1. Launch Visual Studio.
2. Choose "Open a local folder".
3. Select the folder holding the checked out Archon project.
4. Wait for Visual Studio to finish CMake generation.
5. From the "Test" menu, choose "Run CTests for Archon".

Alternatively, after CMake generation has finished:
1. Open `src/archon/test.cpp` inside Visual Studio, and make it the active window.
2. From the "Debug" menu, choose "Start Without Debugging".

Alternatively, if Git for Windows is available, use the [convenience shell
script](#convenience-shell-script).


## Installing Dependencies

### Ubuntu Linux

Run this command to install `libpng`, `libjpeg`, FreeType, Xlib, GLX, OpenGL, and GLEW:

```sh
sudo apt install libpng-dev libjpeg-dev libfreetype-dev libx11-dev libxi-dev libxfixes-dev libxext-dev libxrandr-dev libxrender-dev libglx-dev libgl-dev libglew-dev
```

Since Ubuntu 25.10, it is possible to install SDL 3 using:

```sh
sudo apt install libsdl3-dev
```

On Ubuntu 24.04, one can install SDL 3 by building it from source. Check out the SDL source
code from GitHub using:

```sh
git clone https://github.com/libsdl-org/SDL.git
```

Install build dependencies using:

```sh
sudo apt install libxinerama-dev libxcursor-dev libxss-dev libwayland-dev wayland-protocols libdecor-0-dev libegl-dev libdrm-dev libgbm-dev libvulkan-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libsndio-dev libusb-1.0-0-dev libudev-dev libxkbcommon-dev libibus-1.0-dev
```

Finally, to configure, build, and install SDL, enter the directory in which SDL was checked
out, then run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSDL_X11_XTEST=OFF
cmake --build build -j
sudo cmake --install build
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


## Convenience Shell Script

The Archon project provides a shell script called `do.sh`. Its purpose is to make it a
little bit less tedious to execute the commands needed to configure, build, and test the
Archon libraries. It also has functionality to execute the demo programs that are associated
with the libraries. See [Demo Programs](#demo-programs) below.

The shell script is written for a POSIX shell and works in general on POSIX platforms. It
also works on Windows inside a Cygwin-like environment such as Git Bash (see the
Windows-specific notes below).

To build and execute the test suite, run this command:

```sh
bash do.sh check-debug
```

This builds and executes the Archon test command (`build/debug/src/test`). Any additional
command line arguments will be passed directly to that command. Pass `--help` to see a list
of command-line options (there are a lot).

There is a plethora of possible variations on `check-debug` that also execute the test
command but in different ways: built in debug or release mode, with or without various
sanitizers (address, thread, ...), inside a debugger (`gdb`, `lldg`), and more.

To see a complete list of functions provided by `do.sh`, run this command:

```sh
bash do.sh help
```


### Demo Programs

Some of the libraries have demo programs associated with them. For example, the Font library
has `archon-render-text` with source code located in
[`src/archon/font/demo/render_text.cpp`](src/archon/font/demo/render_text.cpp). Demo
programs are generally located in a `demo` subdirectory inside the main directory of the
library.

You can conveniently build and execute `archon-render-text` by running this command:

```sh
bash do.sh run-debug src/archon/font/demo/archon-render-text "Hello" /tmp/out.png
```

Note that additional command-line arguments are passed to the built demo program.

As was the case for `check-debug` (see above), there is a plethora of available variations
on `run-debug`: built in debug or release mode, with or without various sanitizers (address,
thread, ...), inside a debugger (`gdb`, `lldg`), and more. Use `bash do.sh help` to see them
all.


### Windows

If Git Bash, which is part of Git for Windows[^gitfw], or another Cygwin-like environment is
installed, the convenience shell script can be used.

The script assumes that the dependencies were installed using Vcpkg (see [Installing
Dependencies](#installing-dependencies)), and that the `vcpkg` command can be found via the
`PATH` environment variable. It also assumes that the `cmake` command can be found via the
`PATH` environment variable.

To use the CMake installation that is part of Visual Studio, add `<cmake root>\bin` to the
`PATH` environment variable using the system tool, "Edit the system environment
variables". For the community edition of Visual Studio 2022, `<cmake path>` might be
`C:\Program Files\Microsoft Visual
Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake`. For the
community edition of Visual Studio 2026, it might instead be `C:\Program Files\Microsoft
Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake`.

[^gitfw]: Git for Windows can be installed from https://git-scm.com/download/win. See also
    https://gitforwindows.org/.
