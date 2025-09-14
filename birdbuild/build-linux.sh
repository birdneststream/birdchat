#!/bin/bash
#
# BirdChat Linux Build Script
# 
# This script builds the Linux version of BirdChat with all plugins.
# This script should only be called from build-release.sh, not run directly.
#

# Ensure this script is called from build-release.sh
if [ -z "$SCRIPT_DIR" ] || [ -z "$DOCKER_IMAGE" ] || [ -z "$SOURCE_DIR" ]; then
    echo "ERROR: This script must be called from build-release.sh"
    echo "Usage: ./build-release.sh linux"
    exit 1
fi

build_linux() {
    log_info "Building Linux version with all plugins..."
    
    docker run --rm --user $(id -u):$(id -g) -v "$SCRIPT_DIR/..:/workspace" "$DOCKER_IMAGE" bash -c "
        set -e
        
        # Reset environment for native Linux build
        unset PKG_CONFIG_PATH CC
        
        # Configure native Linux build
        echo '[INFO] Configuring Linux build...'
        meson setup builddir-linux -Dplugin=true \\
            -Dwith-winamp=false -Dwith-sysinfo=true \\
            -Dwith-lua=luajit -Dwith-python=python3 -Dwith-perl=false \\
            -Ddbus=enabled -Dlibcanberra=enabled
        
        # Compile
        echo '[INFO] Compiling Linux build...'
        meson compile -C builddir-linux
        
        # Create Linux bundle
        echo '[INFO] Creating Linux bundle...'
        mkdir -p linux-bundle/bin linux-bundle/plugins linux-bundle/share
        
        # Copy main executable
        cp builddir-linux/src/fe-gtk/birdchat linux-bundle/bin/
        
        # Copy plugins
        cp builddir-linux/plugins/fishlim/fishlim.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] fishlim plugin not found'
        cp builddir-linux/plugins/checksum/checksum.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] checksum plugin not found'
        cp builddir-linux/plugins/exec/exec.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] exec plugin not found'
        cp builddir-linux/plugins/sysinfo/sysinfo.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] sysinfo plugin not found'
        cp builddir-linux/plugins/lua/lua.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] lua plugin not found'
        cp builddir-linux/plugins/python/python.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] python plugin not found'
        
        # Copy other built assets if they exist
        if [ -d builddir-linux/po ]; then
            cp -r builddir-linux/po linux-bundle/share/ 2>/dev/null || true
        fi
        
        # Create launch script
        cat > linux-bundle/birdchat << 'LAUNCH_EOF'
#!/bin/bash
SCRIPT_DIR=\"\$(cd \"\$(dirname \"\${BASH_SOURCE[0]}\")\" && pwd)\"
export HEXCHAT_PLUGIN_PATH=\"\$SCRIPT_DIR/plugins\"
exec \"\$SCRIPT_DIR/bin/birdchat\" \"\$@\"
LAUNCH_EOF
        chmod +x linux-bundle/birdchat
        
        echo '[SUCCESS] Linux build complete!'
        echo '[INFO] Bundle contents:'
        ls -la linux-bundle/
        echo '[INFO] Plugins:'
        ls -la linux-bundle/plugins/ 2>/dev/null || echo 'No plugins directory'
    "
    
    log_success "Linux build completed"
}

# Call the function
build_linux