# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BirdChat is a fork of HexChat (IRC client) with additional features including 99 color support. It's an IRC client built for Windows and Unix-like systems using C with GTK for the GUI.

## Build System

BirdChat uses Meson as its build system. The main build configuration is in `meson.build`.

### Common Build Commands

```bash
# Configure build directory
meson setup build

# Compile the project
meson compile -C build

# Install (requires appropriate permissions)
meson install -C build

# Configure with specific options
meson setup build -Dgtk-frontend=true -Dplugin=true

# Run tests if available
meson test -C build
```

### Build Options

Key options from `meson_options.txt`:
- `gtk-frontend`: Main graphical interface (default: true)
- `text-frontend`: Text interface (default: false)
- `plugin`: Plugin support (default: varies)
- `tls`: TLS/SSL support via OpenSSL (default: enabled)
- `with-lua`: Lua scripting (default: luajit)
- `with-python`: Python scripting (default: python3)
- `with-perl`: Perl scripting (default: perl)

## Code Architecture

### Core Components

- **src/common/**: Core IRC protocol handling and shared functionality
  - `hexchat.c/h`: Main application logic and data structures
  - `server.c/h`: IRC server connection management
  - `proto-irc.c/h`: IRC protocol implementation
  - `inbound.c/h` & `outbound.c/h`: Message handling
  - `plugin.c/h`: Plugin system implementation

- **src/fe-gtk/**: GTK-based graphical frontend
  - Main GUI implementation

- **src/fe-text/**: Text-based frontend
  - Console interface implementation

### Plugin System

- **plugins/**: Plugin directory containing:
  - `python/`: Python scripting plugin
  - `perl/`: Perl scripting plugin
  - `lua/`: Lua scripting plugin
  - `fishlim/`: Fish encryption plugin
  - `sysinfo/`: System information plugin
  - `checksum/`: DCC checksum plugin

### Platform-Specific

- **win32/**: Windows-specific build files and configurations
- **osx/**: macOS-specific files including bundle configuration
- **flatpak/**: Flatpak packaging configuration

## Key Implementation Details

- The project uses GTK 2 with GLib for cross-platform compatibility
- OpenSSL is used for TLS/SSL support
- The codebase follows C89 standard (`c_std=gnu89`)
- IRC protocol color codes have been extended to support 99 colors (non-standard feature)
- Plugin API is defined in `src/common/hexchat-plugin.h`

## Development Notes

- The application still uses "hexchat" in many internal identifiers and configuration despite the rename to BirdChat
- Configuration data uses GLib's configuration system
- Internationalization is handled through gettext (po/ directory)
- The project includes security hardening flags in the build configuration