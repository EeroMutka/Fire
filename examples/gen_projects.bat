:: This script compiles and runs the "gen_projects.c" file and cleans up any files left behind.
:: All project configuration is done inside the "gen_projects.c" file - read the comment at the
:: top of the file for more info.

@echo off

:: Unfortunately, Microsoft doesn't let us easily use their C/C++ compiler from the command line.
:: Instead, you must open "X64 Native Tools Command Prompt for VS" and manually navigate to the
:: target folder. An alternative is to manually call the vcvarsall.bat script first. The following
:: section finds and calls the vcvarsall.bat script if it hasn't been called yet.
setlocal enabledelayedexpansion
where /Q cl.exe || (
  @echo -- Calling vcvarsall.bat --
  for /f "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set VS=%%i
  if "!VS!" equ "" (
    echo ERROR: Visual Studio installation not found
    exit /b 1
  ) 
  call "!VS!\VC\Auxiliary\Build\vcvarsall.bat" amd64 || exit /b 1
)

@echo -- Generating projects --

cl gen_projects.c && gen_projects.exe
del gen_projects.obj
del gen_projects.exe

@echo -- DONE --