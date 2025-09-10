#!/bin/bash
#
# BirdChat Release Build Script
# 
# This script automates the complete build process for BirdChat on both Linux and Windows platforms.
# It handles Docker environment setup, cross-platform compilation, plugin building, and release packaging.
#
# Usage:
#   ./build-release.sh [linux|windows|all] [--clean] [--docker-rebuild]
#
# Examples:
#   ./build-release.sh all                    # Build both platforms
#   ./build-release.sh windows --clean        # Clean Windows build
#   ./build-release.sh linux --docker-rebuild # Rebuild Docker image and Linux
#

set -e  # Exit on any error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_IMAGE="birdchat-build-universal"
RELEASES_DIR="$SCRIPT_DIR/releases"
BUILD_TIMESTAMP=$(date +"%Y%m%d-%H%M%S")
VERSION=$(grep -E "^\s*version\s*:" "$SCRIPT_DIR/meson.build" | head -1 | sed -E "s/.*version\s*:\s*'([^']+)'.*/\1/" || echo "dev")

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

# Build Docker image
build_docker_image() {
    log_info "Building Docker image '$DOCKER_IMAGE'..."
    log_info "This may take 5-10 minutes on first run..."
    
    docker build -t "$DOCKER_IMAGE" - <<'EOF'
FROM fedora:latest
RUN dnf install -y \
    # MinGW cross-compilation tools for Windows
    mingw64-gcc mingw64-gtk3 mingw64-glib2 meson ninja-build pkg-config \
    mingw64-pkg-config mingw64-openssl glib2-devel gettext gdk-pixbuf2-devel \
    mingw64-python3 perl-devel lua-devel \
    # Native Linux development tools
    gtk3-devel openssl-devel \
    # Archive tools for packaging
    zip tar gzip \
    && dnf clean all

# Default Windows cross-compilation environment
ENV PKG_CONFIG_PATH=/usr/x86_64-w64-mingw32/sys-root/mingw/lib/pkgconfig
ENV CC=x86_64-w64-mingw32-gcc
WORKDIR /workspace
EOF
    
    log_success "Docker image built successfully"
}

# Setup Docker environment
setup_docker() {
    check_docker
    
    if [[ "$DOCKER_REBUILD" == "true" ]] || ! check_docker_image; then
        build_docker_image
    fi
}

# Clean build directories
clean_builds() {
    local platform="$1"
    
    case "$platform" in
        "windows")
            log_info "Cleaning Windows build directory..."
            rm -rf "$SCRIPT_DIR/builddir-windows"
            rm -rf "$SCRIPT_DIR/windows-bundle-ultimate"
            ;;
        "linux")
            log_info "Cleaning Linux build directory..."
            rm -rf "$SCRIPT_DIR/builddir-linux"
            rm -rf "$SCRIPT_DIR/linux-bundle"
            ;;
        "all")
            log_info "Cleaning all build directories..."
            rm -rf "$SCRIPT_DIR/builddir-windows" "$SCRIPT_DIR/builddir-linux"
            rm -rf "$SCRIPT_DIR/windows-bundle-ultimate" "$SCRIPT_DIR/linux-bundle"
            ;;
    esac
}

# Build Windows version
build_windows() {
    log_info "Building Windows version with all plugins..."
    
    docker run --rm -v "$SCRIPT_DIR:/workspace" "$DOCKER_IMAGE" bash -c "
        set -e
        
        # Configure Windows build with all plugins
        echo '[INFO] Configuring Windows build...'
        meson setup builddir-windows --cross-file=/usr/share/mingw/toolchain-mingw64.meson \\
            -Dwith-winamp=true -Dwith-sysinfo=true -Dwith-upd=false \\
            -Dwith-lua=false -Dwith-python=false -Dwith-perl=false -Dplugin=true
        
        # Compile
        echo '[INFO] Compiling Windows build...'
        meson compile -C builddir-windows
        
        # Create complete Windows bundle
        echo '[INFO] Creating Windows bundle...'
        mkdir -p windows-bundle-ultimate
        cp builddir-windows/src/fe-gtk/birdchat.exe windows-bundle-ultimate/
        
        # Copy all plugins
        mkdir -p windows-bundle-ultimate/plugins
        cp builddir-windows/plugins/fishlim/fishlim.dll windows-bundle-ultimate/plugins/ 2>/dev/null || echo '[WARNING] fishlim.dll not found'
        cp builddir-windows/plugins/checksum/checksum.dll windows-bundle-ultimate/plugins/ 2>/dev/null || echo '[WARNING] checksum.dll not found'
        cp builddir-windows/plugins/exec/libexec.dll windows-bundle-ultimate/plugins/exec.dll 2>/dev/null || echo '[WARNING] exec.dll not found'
        cp builddir-windows/plugins/winamp/winamp.dll windows-bundle-ultimate/plugins/ 2>/dev/null || echo '[WARNING] winamp.dll not found'
        cp builddir-windows/plugins/sysinfo/sysinfo.dll windows-bundle-ultimate/plugins/ 2>/dev/null || echo '[WARNING] sysinfo.dll not found'
        
        # Copy all system DLLs
        echo '[INFO] Copying system DLLs...'
        cp /usr/x86_64-w64-mingw32/sys-root/mingw/bin/*.dll windows-bundle-ultimate/ 2>/dev/null || true
        
        echo '[SUCCESS] Windows build complete!'
        echo '[INFO] Bundle contents:'
        ls -la windows-bundle-ultimate/
        echo '[INFO] Plugins:'
        ls -la windows-bundle-ultimate/plugins/ 2>/dev/null || echo 'No plugins directory'
    "
    
    log_success "Windows build completed"
}

# Build Linux version
build_linux() {
    log_info "Building Linux version with all plugins..."
    
    docker run --rm -v "$SCRIPT_DIR:/workspace" "$DOCKER_IMAGE" bash -c "
        set -e
        
        # Reset environment for native Linux build
        unset PKG_CONFIG_PATH CC
        
        # Configure native Linux build
        echo '[INFO] Configuring Linux build...'
        meson setup builddir-linux -Dplugin=true \\
            -Dwith-winamp=false -Dwith-sysinfo=true \\
            -Dwith-lua=false -Dwith-python=false -Dwith-perl=false
        
        # Compile
        echo '[INFO] Compiling Linux build...'
        meson compile -C builddir-linux
        
        # Create Linux bundle
        echo '[INFO] Creating Linux bundle...'
        mkdir -p linux-bundle/bin linux-bundle/plugins linux-bundle/share
        
        # Copy main executable
        cp builddir-linux/src/fe-gtk/birdchat linux-bundle/bin/
        
        # Copy plugins
        cp builddir-linux/plugins/fishlim/libfishlim.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] fishlim plugin not found'
        cp builddir-linux/plugins/checksum/libchecksum.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] checksum plugin not found'
        cp builddir-linux/plugins/exec/libexec.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] exec plugin not found'
        cp builddir-linux/plugins/sysinfo/libsysinfo.so linux-bundle/plugins/ 2>/dev/null || echo '[WARNING] sysinfo plugin not found'
        
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

# Package releases
package_releases() {
    local platform="$1"
    
    # Create releases directory
    mkdir -p "$RELEASES_DIR"
    
    case "$platform" in
        "windows")
            if [ -d "$SCRIPT_DIR/windows-bundle-ultimate" ]; then
                local zip_name="birdchat-${VERSION}-windows-${BUILD_TIMESTAMP}.zip"
                log_info "Packaging Windows release: $zip_name"
                
                cd "$SCRIPT_DIR"
                zip -r "$RELEASES_DIR/$zip_name" windows-bundle-ultimate/
                
                log_success "Windows release packaged: $RELEASES_DIR/$zip_name"
                log_info "Size: $(du -h "$RELEASES_DIR/$zip_name" | cut -f1)"
            else
                log_error "Windows bundle directory not found"
                return 1
            fi
            ;;
        "linux")
            if [ -d "$SCRIPT_DIR/linux-bundle" ]; then
                local tar_name="birdchat-${VERSION}-linux-${BUILD_TIMESTAMP}.tar.gz"
                log_info "Packaging Linux release: $tar_name"
                
                cd "$SCRIPT_DIR"
                tar -czf "$RELEASES_DIR/$tar_name" linux-bundle/
                
                log_success "Linux release packaged: $RELEASES_DIR/$tar_name"
                log_info "Size: $(du -h "$RELEASES_DIR/$tar_name" | cut -f1)"
            else
                log_error "Linux bundle directory not found"
                return 1
            fi
            ;;
    esac
}

# Show usage
show_usage() {
    cat << EOF
BirdChat Release Build Script

Usage: $0 [PLATFORM] [OPTIONS]

PLATFORMS:
    linux       Build Linux version only
    windows     Build Windows version only
    all         Build both platforms (default)

OPTIONS:
    --clean             Clean build directories before building
    --docker-rebuild    Force rebuild of Docker image
    --help              Show this help message

EXAMPLES:
    $0                          # Build both platforms
    $0 all                      # Build both platforms
    $0 windows --clean          # Clean and build Windows only
    $0 linux --docker-rebuild   # Rebuild Docker and build Linux only

OUTPUT:
    All releases are created in: $RELEASES_DIR/
    - Windows: birdchat-VERSION-windows-TIMESTAMP.zip
    - Linux:   birdchat-VERSION-linux-TIMESTAMP.tar.gz

EOF
}

# Main execution
main() {
    local platform="all"
    local clean=false
    DOCKER_REBUILD=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            linux|windows|all)
                platform="$1"
                shift
                ;;
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
    
    log_info "BirdChat Release Build Script"
    log_info "Platform: $platform"
    log_info "Version: $VERSION"
    log_info "Timestamp: $BUILD_TIMESTAMP"
    log_info "Working directory: $SCRIPT_DIR"
    log_info "Releases directory: $RELEASES_DIR"
    echo
    
    # Setup Docker environment
    setup_docker
    
    # Clean if requested
    if [[ "$clean" == "true" ]]; then
        clean_builds "$platform"
    fi
    
    # Build platforms
    case "$platform" in
        "windows")
            build_windows
            package_releases "windows"
            ;;
        "linux")
            build_linux
            package_releases "linux"
            ;;
        "all")
            build_windows
            build_linux
            package_releases "windows"
            package_releases "linux"
            ;;
    esac
    
    # Summary
    echo
    log_success "Build process completed!"
    log_info "Released packages:"
    if [ -d "$RELEASES_DIR" ]; then
        ls -la "$RELEASES_DIR"/*${BUILD_TIMESTAMP}* 2>/dev/null || log_warning "No release packages found"
    fi
    
    echo
    log_info "Deployment instructions:"
    log_info "Windows: Extract zip file and run birdchat.exe"
    log_info "Linux:   Extract tar.gz and run ./birdchat"
}

# Run main function
main "$@"