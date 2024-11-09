# Fire Libraries

This is a collection of easy to use programming libraries that are compatible with both C and C++. The following libraries are included:

* `fire_ui/`
  * Immediate-mode user interface library that lets you easily create intuitive UIs with buttons, text boxes, color pickers and lots more!
  * Depends on `fire_ds.h`, `fire_string.h` and [stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h)
  * For a more details, see [my article](https://eeromutka.github.io/projects/imgui)
* `fire_ds.h`
  * Basic data structures and utilities:
    * Memory arena
    * Resizing dynamic array
    * Hash map & set
    * Bucket array
* `fire_string.h`
  * Length-based string type and string utilities
* `fire_os_window.h`
  * Create windows and get keyboard & mouse input (Windows-only for now)
* `fire_os_sync.h`
  * Create threads, mutexes and condition variables (Windows-only for now)
* `fire_os_clipboard.h`
  * Clipboard utilities (Windows-only for now)
* `fire_build.h`
  * Compile C/C++ projects from code (Windows-only for now)

## Building the Examples

Open `examples/build/examples.sln` in [Visual Studio 2017](https://visualstudio.microsoft.com/) (or higher) to build and run the examples.

The VS project files can be configured using the `examples/gen_projects.c` file. To regenerate the project files, run `examples/gen_projects.bat`.

## Screenshots

![ui_demo](/screenshots/ui_demo.png)
