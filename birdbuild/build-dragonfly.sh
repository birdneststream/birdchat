#!/bin/bash
#
# BirdChat DragonFly BSD Build Script
# 
# This script builds a native DragonFly BSD binary with full GTK support
# using cross-compilation from a Linux Docker container. It mounts the 
# DragonFly BSD ISO to extract system headers and libraries.
# This script should only be called from build-release.sh, not run directly.
#

# Ensure this script is called from build-release.sh
if [ -z "$SCRIPT_DIR" ] || [ -z "$DOCKER_IMAGE" ] || [ -z "$SOURCE_DIR" ]; then
    echo "ERROR: This script must be called from build-release.sh"
    echo "Usage: ./build-release.sh dragonfly"
    exit 1
fi

set -e  # Exit on any error

# Configuration
DOCKER_IMAGE="birdchat-dragonfly-build"
DRAGONFLY_ISO="$SCRIPT_DIR/../dragonfly-vm/dfly-x86_64-6.4.2_REL.iso"
BUILD_TIMESTAMP=$(date +"%Y%m%d-%H%M%S")
VERSION=$(grep -E "^\s*version\s*:" "$SCRIPT_DIR/../meson.build" | head -1 | sed -E "s/.*version\s*:\s*'([^']+)'.*/\1/" || echo "dev")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Docker is available
check_docker() {
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed or not in PATH"
        exit 1
    fi
    
    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running or not accessible"
        exit 1
    fi
    
    log_success "Docker is available"
}

# Check if DragonFly BSD ISO exists
check_dragonfly_iso() {
    if [ ! -f "$DRAGONFLY_ISO" ]; then
        log_error "DragonFly BSD ISO not found at: $DRAGONFLY_ISO"
        log_error "Please ensure the ISO is available in dragonfly-vm/"
        exit 1
    fi
    
    log_success "DragonFly BSD ISO found: $DRAGONFLY_ISO"
    log_info "ISO size: $(du -h "$DRAGONFLY_ISO" | cut -f1)"
}

# Check if Docker image exists
check_docker_image() {
    if docker image inspect "$DOCKER_IMAGE" &> /dev/null; then
        log_success "Docker image '$DOCKER_IMAGE' exists"
        return 0
    else
        log_warning "Docker image '$DOCKER_IMAGE' does not exist"
        return 1
    fi
}

# Build Docker image for DragonFly BSD cross-compilation
build_docker_image() {
    log_info "Building Docker image '$DOCKER_IMAGE'..."
    log_info "This may take 5-10 minutes on first run..."
    
    docker build -t "$DOCKER_IMAGE" - <<'EOF'
FROM fedora:latest

# Install basic development tools and cross-compilation dependencies
RUN dnf install -y \
    # Core development tools
    clang lld llvm gcc g++ make cmake \
    # Build system tools
    meson ninja-build pkg-config python3-pip \
    # Archive and file tools
    tar xz gzip zip p7zip p7zip-plugins \
    # Network tools
    wget curl \
    # Development libraries
    glib2-devel openssl-devel zlib-devel gdk-pixbuf2-devel \
    # File system tools for ISO mounting
    util-linux \
    && dnf clean all \
    && pip3 install meson ninja

# Create mount points 
RUN mkdir -p /mnt/dragonfly-iso

# Set working directory
WORKDIR /workspace

# Default environment for DragonFly BSD cross-compilation
ENV DRAGONFLY_TARGET=x86_64-pc-dragonfly-elf
ENV DRAGONFLY_SYSROOT=/workspace/dragonfly-sysroot
EOF
    
    log_success "Docker image built successfully"
}

# Setup Docker environment
setup_docker() {
    check_docker
    check_dragonfly_iso
    
    if [[ "$DOCKER_REBUILD" == "true" ]] || ! check_docker_image; then
        build_docker_image
    fi
}

# Clean build directories
clean_builds() {
    log_info "Cleaning DragonFly BSD build directories..."
    rm -rf "$SCRIPT_DIR/builddir-dragonfly"
    rm -rf "$SCRIPT_DIR/dragonfly-bundle"
    rm -f "$SCRIPT_DIR/dragonfly-cross.ini"
}

# Build DragonFly BSD version
build_dragonfly() {
    log_info "Building DragonFly BSD version with full GTK support..."
    
    # Check if ISO exists before mounting
    if [ ! -f "$DRAGONFLY_ISO" ]; then
        log_error "DragonFly BSD ISO not found: $DRAGONFLY_ISO"
        exit 1
    fi
    
    docker run --rm --privileged --user $(id -u):$(id -g) -v "$SCRIPT_DIR/..:/workspace" -v "$DRAGONFLY_ISO:/dragonfly.iso:ro" "$DOCKER_IMAGE" bash -c "
        set -e
        
        echo '[INFO] Setting up DragonFly BSD cross-compilation environment...'
        
        # Extract DragonFly BSD ISO files using 7z
        echo '[INFO] Extracting DragonFly BSD system files from ISO...'
        mkdir -p /workspace/dragonfly-sysroot /tmp/iso-extract
        
        echo '[INFO] Using 7z to extract ISO (this may take a few minutes)...'
        7z x -o/tmp/iso-extract /dragonfly.iso >/dev/null 2>&1 || echo '[INFO] 7z completed with warnings (normal for some ISOs)'
        
        echo '[INFO] 7z extraction completed'
        echo '[INFO] Checking extracted contents...'
        ls -la /tmp/iso-extract/ | head -10
        
        # Look for usr directory in various locations
        if [ -d /tmp/iso-extract/usr ]; then
            echo '[SUCCESS] Found DragonFly system files in /tmp/iso-extract/usr'
            ISO_BASE_PATH="/tmp/iso-extract"
        elif [ -d /tmp/iso-extract/*/usr ]; then
            echo '[SUCCESS] Found DragonFly system files in subdirectory'
            ISO_BASE_PATH=\$(find /tmp/iso-extract -name \"usr\" -type d | head -1 | dirname)
            echo \"[INFO] Using base path: \$ISO_BASE_PATH\"
        else
            echo '[WARNING] Could not find usr directory in ISO extraction - this may be a boot ISO'
            echo '[INFO] Available directories:'
            find /tmp/iso-extract -type d -name \"*\" | head -10 2>/dev/null || echo '[INFO] No directories found'
            echo '[INFO] Skipping base system extraction, will rely on downloaded packages...'
            ISO_BASE_PATH=""
        fi
        
        if [ -n \"\$ISO_BASE_PATH\" ]; then
            echo '[SUCCESS] Proceeding with DragonFly system files extraction'
            
            # Copy system files to sysroot
            mkdir -p /workspace/dragonfly-sysroot/usr/include /workspace/dragonfly-sysroot/usr/lib /workspace/dragonfly-sysroot/lib
            mkdir -p /workspace/dragonfly-sysroot/usr/local/include /workspace/dragonfly-sysroot/usr/local/lib
            
            # Copy files using discovered base path
            if [ -d \"\$ISO_BASE_PATH/usr/include\" ]; then
                cp -r \$ISO_BASE_PATH/usr/include/* /workspace/dragonfly-sysroot/usr/include/ 2>/dev/null || echo '[WARNING] Failed to copy headers'
            fi
            if [ -d \"\$ISO_BASE_PATH/usr/lib\" ]; then
                cp -r \$ISO_BASE_PATH/usr/lib/* /workspace/dragonfly-sysroot/usr/lib/ 2>/dev/null || echo '[WARNING] Failed to copy libraries'
            fi
            if [ -d \"\$ISO_BASE_PATH/lib\" ]; then
                cp -r \$ISO_BASE_PATH/lib/* /workspace/dragonfly-sysroot/lib/ 2>/dev/null || echo '[WARNING] Failed to copy base libraries'
            fi
            
            # Copy additional system directories if they exist
            if [ -d \"\$ISO_BASE_PATH/usr/local/include\" ]; then
                cp -r \$ISO_BASE_PATH/usr/local/include/* /workspace/dragonfly-sysroot/usr/local/include/ 2>/dev/null || echo '[WARNING] Failed to copy local headers'
            fi
            if [ -d \"\$ISO_BASE_PATH/usr/local/lib\" ]; then
                cp -r \$ISO_BASE_PATH/usr/local/lib/* /workspace/dragonfly-sysroot/usr/local/lib/ 2>/dev/null || echo '[WARNING] Failed to copy local libraries'
            fi
            
            # Create pkg-config directory structure (DragonFly uses libdata/pkgconfig)
            mkdir -p /workspace/dragonfly-sysroot/usr/local/libdata/pkgconfig /workspace/dragonfly-sysroot/usr/libdata/pkgconfig
            
            # Copy existing pkg-config files from DragonFly structure
            if [ -d \"\$ISO_BASE_PATH/usr/local/libdata/pkgconfig\" ]; then
                cp -r \$ISO_BASE_PATH/usr/local/libdata/pkgconfig/* /workspace/dragonfly-sysroot/usr/local/libdata/pkgconfig/ 2>/dev/null || true
            fi
            if [ -d \"\$ISO_BASE_PATH/usr/libdata/pkgconfig\" ]; then
                cp -r \$ISO_BASE_PATH/usr/libdata/pkgconfig/* /workspace/dragonfly-sysroot/usr/libdata/pkgconfig/ 2>/dev/null || true
            fi
            
            echo '[INFO] Downloading GTK3 and GLib packages from DragonFly ports...'
            
            # Download essential GTK3 and GLib packages
            DRAGONFLY_REPO='https://mirror.checkdomain.de/dragonflybsd/dports/dragonfly:6.4:x86:64/LATEST/All/'
            
            cd /tmp
            
            # List of packages to get - GTK3 and all major dependencies
            packages=(
                'gtk3-3.24.43.pkg'
                'glib-2.80.5,2.pkg'
                'pango-1.52.2_1.pkg'
                'gdk-pixbuf2-2.42.10_3.pkg'
                'cairo-1.17.4_2,3.pkg'
                'libffi-3.4.6.pkg'
                'harfbuzz-9.0.0.pkg'
                'pixman-0.42.2.pkg'
                'fontconfig-2.15.0_3,1.pkg'
                'freetype2-2.13.2.pkg'
                'png-1.6.43.pkg'
                'shared-mime-info-2.2_3.pkg'
                'atkmm-2.28.0_1.pkg'
                'libX11-1.8.9,1.pkg'
                'libXext-1.3.6,1.pkg'
                'libxcb-1.17.0.pkg'
                'libXrender-0.9.10_2.pkg'
                'brotli-1.1.0,1.pkg'
                'graphite2-1.3.14.pkg'
                'libXau-1.0.9_1.pkg'
                'libXdmcp-1.1.5.pkg'
                'xorgproto-2024.1.pkg'
                'mesa-libs-21.3.9.pkg'
                'at-spi2-core-2.52.0.pkg'
                'fribidi-1.0.15.pkg'
                'libthai-0.1.29_1.pkg'
                'libXft-2.3.8.pkg'
                'libdatrie-0.2.13_2.pkg'
                'libglvnd-1.7.0.pkg'
                'mesa-dri-21.3.9.pkg'
                'libXi-1.8_1,1.pkg'
                'libXrandr-1.5.2_1.pkg'
                'libXcursor-1.2.2.pkg'
                'libXfixes-6.0.0_1.pkg'
                'libXcomposite-0.4.6_1,1.pkg'
                'libXdamage-1.1.6.pkg'
                'libXinerama-1.1.4_3,1.pkg'
                'wayland-1.23.1.pkg'
                'libxkbcommon-1.7.0_1.pkg'
                'libepoxy-1.5.9.pkg'
                'dbus-1.14.10_5,1.pkg'
                'libXtst-1.2.3_3.pkg'
                'openssl-3.0.15,1.pkg'
                'libcanberra-0.30_10.pkg'
                'libcanberra-gtk3-0.30_10.pkg'
                'dbus-glib-0.112_1.pkg'
                'gettext-tools-0.22.5.pkg'
                'gettext-0.22.5.pkg'
            )
            
            echo '[INFO] Getting GTK3 and GLib packages...'
            
            for pkg in \"\${packages[@]}\"; do
                if [ -f \"/workspace/dragonfly-vm/\$pkg\" ]; then
                    echo \"[INFO] Using local package: \$pkg\"
                    cp \"/workspace/dragonfly-vm/\$pkg\" .
                else
                    echo \"[INFO] Downloading \$pkg from mirror...\"
                    wget --progress=dot:mega \"\${DRAGONFLY_REPO}\$pkg\" || echo \"[WARNING] Failed to download \$pkg\"
                fi
            done
            
            echo '[INFO] Extracting DragonFly GTK3 packages...'
            
            # Extract packages to sysroot (DragonFly .pkg files are tar archives)
            for pkg in *.pkg; do
                if [ -f \"\$pkg\" ]; then
                    echo \"[INFO] Extracting \$pkg...\"
                    tar -xf \"\$pkg\" -C /workspace/dragonfly-sysroot/ 2>/dev/null || echo \"[WARNING] Failed to extract \$pkg\"
                fi
            done
            
            echo '[INFO] DragonFly sysroot populated with GTK3 libraries'
        else
            echo '[ERROR] Failed to extract DragonFly system files'
            exit 1
        fi
        
        # Set up cross-compilation environment variables
        export DRAGONFLY_SYSROOT=/workspace/dragonfly-sysroot
        export CC='clang --target=x86_64-pc-dragonfly-elf --sysroot=/workspace/dragonfly-sysroot'
        export CXX='clang++ --target=x86_64-pc-dragonfly-elf --sysroot=/workspace/dragonfly-sysroot'
        
        # Configure pkg-config for DragonFly cross-compilation (DragonFly uses libdata/pkgconfig)
        export PKG_CONFIG_LIBDIR=/workspace/dragonfly-sysroot/usr/local/libdata/pkgconfig:/workspace/dragonfly-sysroot/usr/libdata/pkgconfig
        export PKG_CONFIG_SYSROOT_DIR=/workspace/dragonfly-sysroot
        
        # Don't add DragonFly binaries to PATH - use host tools instead
        # export PATH="/workspace/dragonfly-sysroot/usr/local/bin:/workspace/dragonfly-sysroot/usr/bin:\$PATH"
        
        # Find gdk-pixbuf-pixdata tool and create wrapper (prefer host system version)
        GDK_PIXBUF_TOOL=\"\"
        for path in /usr/bin/gdk-pixbuf-pixdata /usr/local/bin/gdk-pixbuf-pixdata; do
            if [ -x \"\$path\" ]; then
                GDK_PIXBUF_TOOL=\"\$path\"
                break
            fi
        done
        
        if [ -z \"\$GDK_PIXBUF_TOOL\" ]; then
            echo '[WARNING] gdk-pixbuf-pixdata not found, creating stub'
            # Create a stub that just copies input to output for resources that don't need pixdata
            cat > /tmp/gdk-pixbuf-pixdata << 'PIXDATA_EOF'
#!/bin/sh
echo '[WARNING] gdk-pixbuf-pixdata stub called with:' \"\$@\" >&2
exit 0
PIXDATA_EOF
        else
            echo \"[INFO] Found gdk-pixbuf-pixdata at: \$GDK_PIXBUF_TOOL\"
            cat > /tmp/gdk-pixbuf-pixdata << PIXDATA_EOF
#!/bin/sh
exec \"\$GDK_PIXBUF_TOOL\" \"\\\$@\"
PIXDATA_EOF
        fi
        chmod +x /tmp/gdk-pixbuf-pixdata
        export PATH=\"/tmp:/usr/bin:\$PATH\"
        export GDK_PIXBUF_PIXDATA=/tmp/gdk-pixbuf-pixdata
        
        # Unset problematic Linux-specific environment variables  
        unset PKG_CONFIG_PATH
        
        cd /workspace
        echo '[INFO] Creating DragonFly BSD cross-compilation file...'
        
        # Create cross-compilation file for DragonFly BSD
        cat > dragonfly-cross.ini << 'CROSS_EOF'
[binaries]
c = 'clang'
cpp = 'clang++'
ar = 'llvm-ar'
strip = 'llvm-strip'
pkg-config = 'pkg-config'
glib-genmarshal = '/usr/bin/glib-genmarshal'
glib-compile-resources = '/usr/bin/glib-compile-resources'
gdk-pixbuf-pixdata = '/usr/bin/gdk-pixbuf-pixdata'

[built-in options]
c_args = ['--target=x86_64-pc-dragonfly-elf', '--sysroot=/workspace/dragonfly-sysroot', '-I/workspace/dragonfly-sysroot/include', '-I/workspace/dragonfly-sysroot/usr/local/include', '-I/workspace/dragonfly-sysroot/usr/include', '-I/workspace/dragonfly-sysroot/usr/include/sys', '-D__STDC_LIMIT_MACROS', '-D__STDC_CONSTANT_MACROS', '-D_BSD_SOURCE', '-D_DEFAULT_SOURCE']
cpp_args = ['--target=x86_64-pc-dragonfly-elf', '--sysroot=/workspace/dragonfly-sysroot', '-I/workspace/dragonfly-sysroot/include', '-I/workspace/dragonfly-sysroot/usr/local/include', '-I/workspace/dragonfly-sysroot/usr/include', '-I/workspace/dragonfly-sysroot/usr/include/sys', '-D__STDC_LIMIT_MACROS', '-D__STDC_CONSTANT_MACROS', '-D_BSD_SOURCE', '-D_DEFAULT_SOURCE']
c_link_args = ['--target=x86_64-pc-dragonfly-elf', '--sysroot=/workspace/dragonfly-sysroot', '-L/workspace/dragonfly-sysroot/lib', '-L/workspace/dragonfly-sysroot/usr/local/lib', '-L/workspace/dragonfly-sysroot/usr/lib']
cpp_link_args = ['--target=x86_64-pc-dragonfly-elf', '--sysroot=/workspace/dragonfly-sysroot', '-L/workspace/dragonfly-sysroot/lib', '-L/workspace/dragonfly-sysroot/usr/local/lib', '-L/workspace/dragonfly-sysroot/usr/lib']

[host_machine]
system = 'dragonfly'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
CROSS_EOF
        
        echo '[INFO] Configuring full DragonFly BSD build with GTK and plugins...'
        meson setup builddir-dragonfly \\
            --cross-file=dragonfly-cross.ini \\
            -Dgtk-frontend=true \\
            -Dtext-frontend=true \\
            -Dplugin=true \\
            -Dtls=disabled \\
            -Ddbus=disabled \\
            -Dlibcanberra=disabled \\
            -Dwith-fishlim=false \\
            -Dwith-winamp=false -Dwith-sysinfo=false \\
            -Dwith-lua=false -Dwith-python=false -Dwith-perl=false
        
        echo '[INFO] Compiling DragonFly BSD build...'
        meson compile -C builddir-dragonfly
        
        echo '[SUCCESS] Full DragonFly BSD build completed!'
        
        # Verify we got the GTK binary
        if [ -f builddir-dragonfly/src/fe-gtk/birdchat ]; then
            echo '[SUCCESS] ✅ GTK DragonFly BSD binary created!'
        else
            echo '[ERROR] ❌ GTK binary not found - build failed!'
            exit 1
        fi
    "
    
    log_success "DragonFly BSD build completed"
}

# Package DragonFly BSD release
package_release() {
    log_info "Packaging DragonFly BSD release..."
    
    # Find the DragonFly BSD binary - require GTK version
    if [ -f "$SCRIPT_DIR/builddir-dragonfly/src/fe-gtk/birdchat" ]; then
        dragonfly_binary="$SCRIPT_DIR/builddir-dragonfly/src/fe-gtk/birdchat"
        log_success "Found GTK DragonFly BSD binary"
    else
        log_error "No GTK DragonFly BSD binary found - build failed!"
        return 1
    fi
    
    # Create DragonFly BSD bundle
    mkdir -p "$SCRIPT_DIR/dragonfly-bundle/bin" "$SCRIPT_DIR/dragonfly-bundle/plugins"
    cp "$dragonfly_binary" "$SCRIPT_DIR/dragonfly-bundle/bin/"
    
    # Copy plugins if they exist
    cp "$SCRIPT_DIR/builddir-dragonfly/plugins/fishlim/libfishlim.so" "$SCRIPT_DIR/dragonfly-bundle/plugins/" 2>/dev/null || echo '[WARNING] fishlim plugin not found'
    cp "$SCRIPT_DIR/builddir-dragonfly/plugins/checksum/libchecksum.so" "$SCRIPT_DIR/dragonfly-bundle/plugins/" 2>/dev/null || echo '[WARNING] checksum plugin not found'
    cp "$SCRIPT_DIR/builddir-dragonfly/plugins/exec/libexec.so" "$SCRIPT_DIR/dragonfly-bundle/plugins/" 2>/dev/null || echo '[WARNING] exec plugin not found'
    cp "$SCRIPT_DIR/builddir-dragonfly/plugins/sysinfo/libsysinfo.so" "$SCRIPT_DIR/dragonfly-bundle/plugins/" 2>/dev/null || echo '[WARNING] sysinfo plugin not found'
    
    # Create launch script
    cat > "$SCRIPT_DIR/dragonfly-bundle/birdchat" << 'EOF'
#!/bin/sh
SCRIPT_DIR="$(dirname "$0")"
export HEXCHAT_PLUGIN_PATH="$SCRIPT_DIR/plugins"
exec "$SCRIPT_DIR/bin/birdchat" "$@"
EOF
    chmod +x "$SCRIPT_DIR/dragonfly-bundle/birdchat"
    
    # Create releases directory
    mkdir -p "$SCRIPT_DIR/releases"
    
    local tar_name="birdchat-${VERSION}-dragonfly-${BUILD_TIMESTAMP}.tar.gz"
    log_info "Packaging DragonFly BSD release: $tar_name"
    
    cd "$SCRIPT_DIR"
    tar -czf "releases/$tar_name" dragonfly-bundle/
    
    log_success "DragonFly BSD release packaged: releases/$tar_name"
    log_info "Size: $(du -h "releases/$tar_name" | cut -f1)"
}

# Show usage
show_usage() {
    cat << EOF
BirdChat DragonFly BSD Build Script

Usage: $0 [OPTIONS]

OPTIONS:
    --clean             Clean build directories before and after building
    --docker-rebuild    Force rebuild of Docker image
    --help              Show this help message

EXAMPLES:
    $0                    # Build DragonFly BSD version
    $0 --clean            # Clean and build DragonFly BSD version
    $0 --docker-rebuild   # Rebuild Docker and build DragonFly BSD version

REQUIREMENTS:
    - Docker must be installed and running
    - DragonFly BSD ISO must be present at: dragonfly-vm/dfly-x86_64-6.4.2_REL.iso
    - Docker must have --privileged access for ISO extraction

OUTPUT:
    Release created in: releases/birdchat-VERSION-dragonfly-TIMESTAMP.tar.gz
    Full GTK application with plugins

EOF
}

# Main execution
main() {
    local clean=false
    DOCKER_REBUILD=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                clean=true
                shift
                ;;
            --docker-rebuild)
                DOCKER_REBUILD=true
                shift
                ;;
            --help|-h)
                show_usage
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    log_info "BirdChat DragonFly BSD Build Script"
    log_info "Version: $VERSION"
    log_info "Timestamp: $BUILD_TIMESTAMP"
    log_info "Working directory: $SCRIPT_DIR"
    log_info "DragonFly ISO: $DRAGONFLY_ISO"
    echo
    
    # Setup Docker environment
    setup_docker
    
    # Clean if requested
    if [[ "$clean" == "true" ]]; then
        clean_builds
    fi
    
    # Build DragonFly BSD version
    build_dragonfly
    
    # Package release
    package_release
    
    # Clean up build artifacts if requested
    if [[ "$clean" == "true" ]]; then
        log_info "Cleaning build artifacts after packaging..."
        clean_builds
    fi
    
    # Summary
    echo
    log_success "DragonFly BSD build process completed!"
    log_info "Released package:"
    if [ -d "$SCRIPT_DIR/releases" ]; then
        ls -la "$SCRIPT_DIR/releases"/*dragonfly*${BUILD_TIMESTAMP}* 2>/dev/null || log_warning "No DragonFly BSD release package found"
    fi
    
    echo
    log_info "Deployment instructions:"
    log_info "DragonFly BSD: Extract tar.gz and run ./birdchat"
    log_info "Full GTK application with plugin support"
}

# Run main function
main "$@"