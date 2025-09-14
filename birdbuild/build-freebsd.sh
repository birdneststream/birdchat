#!/bin/bash
#
# BirdChat FreeBSD Build Script
# 
# This script builds the FreeBSD version of BirdChat using cross-compilation.
# This script should only be called from build-release.sh, not run directly.
#

# Ensure this script is called from build-release.sh
if [ -z "$SCRIPT_DIR" ] || [ -z "$DOCKER_IMAGE" ] || [ -z "$SOURCE_DIR" ]; then
    echo "ERROR: This script must be called from build-release.sh"
    echo "Usage: ./build-release.sh freebsd"
    exit 1
fi

build_freebsd() {
    log_info "Building FreeBSD version with full GTK3 and plugins..."
    
    docker run --rm --user $(id -u):$(id -g) -v "$SCRIPT_DIR/..:/workspace" "$DOCKER_IMAGE" bash -c "
        set -e
        
        # Reset environment for FreeBSD cross-compilation
        unset PKG_CONFIG_PATH CC
        
        echo '[INFO] Setting up FreeBSD cross-compilation environment...'
        export FREEBSD_SYSROOT=/opt/freebsd-sysroot
        export CC='clang --target=x86_64-unknown-freebsd14.2 --sysroot=/opt/freebsd-sysroot'
        export CXX='clang++ --target=x86_64-unknown-freebsd14.2 --sysroot=/opt/freebsd-sysroot'
        export PKG_CONFIG_LIBDIR=/opt/freebsd-sysroot/usr/local/lib/pkgconfig:/opt/freebsd-sysroot/usr/lib/pkgconfig
        export PKG_CONFIG_SYSROOT_DIR=/opt/freebsd-sysroot
        
        echo '[INFO] Skipping package download - dependencies should be pre-downloaded'
        
        # Check if we have pre-downloaded packages in /tmp
        cd /tmp
        if ls *.pkg 1> /dev/null 2>&1; then
            echo '[INFO] Found pre-downloaded FreeBSD packages'
            
            echo '[INFO] Extracting FreeBSD packages to sysroot...'
            # Extract packages to sysroot (FreeBSD .pkg files are tar archives)
            for pkg in *.pkg; do
                if [ -f \"\$pkg\" ]; then
                    echo \"[INFO] Extracting \$pkg...\"
                    tar -xf \"\$pkg\" -C /opt/freebsd-sysroot/ 2>/dev/null || echo \"[WARNING] Failed to extract \$pkg\"
                fi
            done
            
            echo '[INFO] FreeBSD sysroot populated with pre-downloaded libraries'
        else
            echo '[WARNING] No pre-downloaded packages found in /tmp'
            echo '[INFO] Please run the dependency download script first'
        fi
        
        # Create wrapper for gdk-pixbuf-pixdata to ensure it can be found
        cat > /tmp/gdk-pixbuf-pixdata << 'PIXDATA_EOF'
#!/bin/sh
exec /usr/bin/gdk-pixbuf-pixdata \"\$@\"
PIXDATA_EOF
        chmod +x /tmp/gdk-pixbuf-pixdata
        export PATH=\"/tmp:/usr/bin:\$PATH\"
        export GDK_PIXBUF_PIXDATA=/tmp/gdk-pixbuf-pixdata
        
        cd /workspace
        echo '[INFO] Configuring full FreeBSD build with GTK3 and plugins...'
        
        # Create full cross-file for FreeBSD
        cat > freebsd-cross.ini << 'CROSS_EOF'
[binaries]
c = 'clang'
cpp = 'clang++'
ar = '/usr/bin/ar'
strip = '/usr/bin/strip'
pkg-config = 'pkg-config'
glib-genmarshal = '/usr/bin/glib-genmarshal'
glib-compile-resources = '/usr/bin/glib-compile-resources'
gdk-pixbuf-pixdata = '/tmp/gdk-pixbuf-pixdata'

[built-in options]
c_args = ['--target=x86_64-unknown-freebsd14.2', '--sysroot=/opt/freebsd-sysroot', '-I/opt/freebsd-sysroot/usr/include', '-I/opt/freebsd-sysroot/usr/local/include']
cpp_args = ['--target=x86_64-unknown-freebsd14.2', '--sysroot=/opt/freebsd-sysroot', '-I/opt/freebsd-sysroot/usr/include', '-I/opt/freebsd-sysroot/usr/local/include']
c_link_args = ['--target=x86_64-unknown-freebsd14.2', '--sysroot=/opt/freebsd-sysroot', '-L/opt/freebsd-sysroot/usr/lib', '-L/opt/freebsd-sysroot/usr/local/lib']
cpp_link_args = ['--target=x86_64-unknown-freebsd14.2', '--sysroot=/opt/freebsd-sysroot', '-L/opt/freebsd-sysroot/usr/lib', '-L/opt/freebsd-sysroot/usr/local/lib']

[host_machine]
system = 'freebsd'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
CROSS_EOF
        
        echo '[INFO] Configuring full FreeBSD build with GTK3 and plugins...'
        meson setup builddir-freebsd \\
            --cross-file=freebsd-cross.ini \\
            -Dgtk-frontend=true \\
            -Dtext-frontend=true \\
            -Dplugin=true \\
            -Dtls=enabled \\
            -Ddbus=disabled \\
            -Dlibcanberra=disabled \\
            -Dwith-fishlim=true \\
            -Dwith-checksum=true \\
            -Dwith-exec=true \\
            -Dwith-winamp=false \\
            -Dwith-sysinfo=false \\
            -Dwith-lua=false \\
            -Dwith-python=false \\
            -Dwith-perl=false
        
        # Compile
        echo '[INFO] Compiling FreeBSD build...'
        meson compile -C builddir-freebsd
        
        # Create FreeBSD bundle
        echo '[INFO] Creating FreeBSD bundle...'
        mkdir -p freebsd-bundle/bin freebsd-bundle/plugins freebsd-bundle/share
        
        # Copy main executable
        cp builddir-freebsd/src/fe-gtk/birdchat freebsd-bundle/bin/ || cp builddir-freebsd/src/fe-text/birdchat freebsd-bundle/bin/
        
        # Copy plugins
        cp builddir-freebsd/plugins/fishlim/libfishlim.so freebsd-bundle/plugins/ 2>/dev/null || echo '[WARNING] fishlim plugin not found'
        cp builddir-freebsd/plugins/checksum/libchecksum.so freebsd-bundle/plugins/ 2>/dev/null || echo '[WARNING] checksum plugin not found'
        cp builddir-freebsd/plugins/exec/libexec.so freebsd-bundle/plugins/ 2>/dev/null || echo '[WARNING] exec plugin not found'
        
        # Copy other built assets if they exist
        if [ -d builddir-freebsd/po ]; then
            cp -r builddir-freebsd/po freebsd-bundle/share/ 2>/dev/null || true
        fi
        
        # Create launch script
        cat > freebsd-bundle/birdchat << 'LAUNCH_EOF'
#!/bin/sh
SCRIPT_DIR=\"\$(dirname \"\$0\")\"
export HEXCHAT_PLUGIN_PATH=\"\$SCRIPT_DIR/plugins\"
exec \"\$SCRIPT_DIR/bin/birdchat\" \"\$@\"
LAUNCH_EOF
        chmod +x freebsd-bundle/birdchat
        
        echo '[SUCCESS] FreeBSD build complete!'
        echo '[INFO] Bundle contents:'
        ls -la freebsd-bundle/
        echo '[INFO] Plugins:'
        ls -la freebsd-bundle/plugins/ 2>/dev/null || echo 'No plugins directory'
    "
    
    log_success "FreeBSD build completed"
}

# Call the function
build_freebsd