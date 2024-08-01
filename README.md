# Fire Libraries

This is a collection of easy to use programming libraries that are compatible with both C and C++. The following libraries are included:

* `fire_ds.h`
  * Basic data structures and utilities, such as memory arenas, dynamic arrays, hash maps and bucketed linked lists
* `fire_string.h`
  * Length-based string type and lots of string utilities
  * Still quite work-in-progress
* `fire_build.h`
  * Lets you easily compile C/C++ projects and generate Visual Studio project files from code
  * Windows-only (for now).
* `fire_os_window.h`
  * Library for creating windows and getting keyboard & mouse input
  * Windows-only (for now)
* `fire_os_sync.h`
  * Library for dealing with threads, mutexes and condition variables
  * Windows-only (for now)
* `fire_os_clipboard.h`
  * Clipboard utilities
  * Windows-only (for now)
* `fire_ui/`
  * Immediate-mode user interface library to give you menus, buttons, text edit boxes and lots more
  * Depends on `fire_ds.h`, `fire_string.h`, [stb_rect_pack.h](https://github.com/nothings/stb/blob/master/stb_rect_pack.h) and [stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h)
  * For a more detailed explanation, see [my article](https://eeromutka.github.io/projects/imgui.html)
  * Still quite work-in-progress

## Building the Examples

Open `examples/build/examples.sln` in [Visual Studio 2017](https://visualstudio.microsoft.com/) (or higher) to build and run the examples.

The VS project files can be edited and generated through a build script found at `examples/gen_project_files.c`. To regenerate the project files,
1. Open `x64 Native Tools Command Prompt for VS <year>` from the Windows Start Menu. Within this terminal, `cd` into the `examples` folder.
2. Run `cl gen_project_files.c && gen_project_files`.
3. Open the newly generated `examples/build/examples.sln` in Visual Studio.

## Screenshots

![ui_demo](/screenshots/ui_demo.png)
