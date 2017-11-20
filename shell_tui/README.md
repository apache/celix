# Shell TUI

The Celix Shell TUI implements a textual user interface for the Celix Shell.

## CMake option
    BUILD_SHELL_TUI=ON

## Config options

- SHELL_USE_ANSI_CONTROL_SEQUENCES - Wether to use ANSI control
sequences to support backspace, left, up, etc key commands in the
shell tui. Default is true if a TERM environment is set else false.

## Using info

If the Celix Shell TUI is installed The `FindCelix.cmake` will set:
 - The `Celix::shell_tui` bundle target if the shell_tui is installed
