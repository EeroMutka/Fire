# Fire Libraries

***WARNING**: Although I use this code myself, it's still experimental work-in-progress and will go through lots more iteration!*

This is a collection of easy-to-use programming libraries that are compatible with both C and C++. The following libraries are included:

* `fire_ds.h`
  * Basic data structures and utilities, such as memory arenas, dynamic arrays, hash maps and bucketed linked lists
* `fire_string.h`
  * Length-based string type and lots of string utilities
* `fire_build.h`
  * Lets you easily compile C/C++ projects and generate Visual Studio project files from code
  * Currently only supports Windows and MSVC
* `fire_os.h`
  * Basic operating system interface, i.e. lets you create windows, get user input, read/write files, etc
  * Currently only supports Windows
* `fire_gpu/`
  * Graphics abstraction layer using Vulkan as a backend
  * The goal is to be a simpler alternative to Vulkan, but to still provide more control than OpenGL/DX11
  * For now, mostly for my own experiments. You're likely better off using an existing graphics API
  * Depends on the [Vulkan SDK](https://vulkan.lunarg.com/) and `fire_ds.h`
* `fire_ui/`
  * Immediate-mode user interface library to give you menus, buttons, text edit boxes and lots more
  * Depends on `fire_ds.h`, `fire_string.h`, [stb_rect_pack.h](https://github.com/nothings/stb/blob/master/stb_rect_pack.h) and [stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h)

## Building the Examples

For now, you'll need Windows and [Visual Studio](https://visualstudio.microsoft.com/) installed to build and run the examples.

1. Install the [Vulkan SDK](https://vulkan.lunarg.com/).
2. Open `examples/gen_projects.c`. On the first line, change the value of VULKAN_SDK_PATH to your vulkan SDK installation path.
3. Open `x64 Native Tools Command Prompt for VS <year>` from the Windows Start Menu. Within this terminal, `cd` into the `examples` folder.
4. Run `cl gen_projects.c && gen_projects`.
5. Open `.build/examples.sln` in Visual Studio and hit build!