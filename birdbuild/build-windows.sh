#!/bin/bash
#
# BirdChat Windows Build Script
# 
# This script builds the Windows version of BirdChat with all plugins.
# This script should only be called from build-release.sh, not run directly.
#

# Ensure this script is called from build-release.sh
if [ -z "$SCRIPT_DIR" ] || [ -z "$DOCKER_IMAGE" ] || [ -z "$SOURCE_DIR" ]; then
    echo "ERROR: This script must be called from build-release.sh"
    echo "Usage: ./build-release.sh windows"
    exit 1
fi

build_windows() {
    log_info "Building Windows version with all plugins..."
    
    docker run --rm --user $(id -u):$(id -g) -v "$SCRIPT_DIR/..:/workspace" "$DOCKER_IMAGE" bash -c "
        set -e
        
        # Configure Windows build with all plugins
        echo '[INFO] Configuring Windows build...'
        meson setup builddir-windows --cross-file=/usr/share/mingw/toolchain-mingw64.meson \\
            -Dwith-winamp=true -Dwith-sysinfo=true -Dwith-upd=false \\
            -Dwith-lua=false -Dwith-python=python3 -Dwith-perl=false -Dplugin=true
        
        # Compile
        echo '[INFO] Compiling Windows build...'
        meson compile -C builddir-windows
        
        # Create complete Windows bundle
        echo '[INFO] Creating Windows bundle...'
        mkdir -p windows-bundle
        cp builddir-windows/src/fe-gtk/birdchat.exe windows-bundle/
        
        # Copy all plugins
        mkdir -p windows-bundle/plugins
        cp builddir-windows/plugins/fishlim/fishlim.dll windows-bundle/plugins/ 2>/dev/null || echo '[WARNING] fishlim.dll not found'
        cp builddir-windows/plugins/checksum/checksum.dll windows-bundle/plugins/ 2>/dev/null || echo '[WARNING] checksum.dll not found'
        cp builddir-windows/plugins/exec/libexec.dll windows-bundle/plugins/exec.dll 2>/dev/null || echo '[WARNING] exec.dll not found'
        cp builddir-windows/plugins/winamp/winamp.dll windows-bundle/plugins/ 2>/dev/null || echo '[WARNING] winamp.dll not found'
        cp builddir-windows/plugins/sysinfo/sysinfo.dll windows-bundle/plugins/ 2>/dev/null || echo '[WARNING] sysinfo.dll not found'
        cp builddir-windows/plugins/python/python.dll windows-bundle/plugins/ 2>/dev/null || echo '[WARNING] python.dll not found'
        
        # Copy all system DLLs
        echo '[INFO] Copying system DLLs...'
        cp /usr/x86_64-w64-mingw32/sys-root/mingw/bin/*.dll windows-bundle/ 2>/dev/null || true
        
        echo '[SUCCESS] Windows build complete!'
        echo '[INFO] Bundle contents:'
        ls -la windows-bundle/
        echo '[INFO] Plugins:'
        ls -la windows-bundle/plugins/ 2>/dev/null || echo 'No plugins directory'
    "
    
    log_success "Windows build completed"
}

# Call the function
build_windows