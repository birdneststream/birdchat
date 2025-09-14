#!/bin/bash

echo "=== HexChat Debug Test Runner ==="
echo ""

# Enable core dumps
ulimit -c unlimited
echo "✅ Core dumps enabled (ulimit -c unlimited)"

# Set up environment
export G_DEBUG=gc-friendly
export G_SLICE=always-malloc
export MALLOC_CHECK_=2
export MALLOC_PERTURB_=1

# GTK Debug options for visual debugging
export GTK_DEBUG=text,interactive

echo "✅ Memory debugging environment set up"
echo "   - G_DEBUG=gc-friendly (better GLib debugging)"
echo "   - G_SLICE=always-malloc (use system malloc)"
echo "   - MALLOC_CHECK_=2 (abort on memory errors)"
echo "   - MALLOC_PERTURB_=1 (initialize freed memory)"
echo "✅ GTK debugging enabled"
echo "   - GTK_DEBUG=text,interactive (text widget internals + inspector)"
echo "   - Press Ctrl+Shift+I to open GTK Inspector"

# Check if we can run with valgrind
if command -v valgrind >/dev/null 2>&1; then
    echo ""
    echo "🔍 Valgrind detected - would you like to run with memory checking? (y/n)"
    read -t 5 -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        echo "🔍 Running with Valgrind (will be slower but catch memory issues)..."
        exec valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
                     --track-origins=yes --verbose --log-file=valgrind.log \
                     ./builddir/src/fe-gtk/birdchat --no-plugins
    fi
fi

echo ""
echo "🚀 Running HexChat with debugging enabled..."
echo "   - All errors will be displayed"
echo "   - Core dump will be generated if crash occurs"
echo "   - Memory errors will cause immediate abort"
echo ""
echo "Press Ctrl+C to stop"
echo "=========================="

# Run with debugging output
stdbuf -oL -eL ./builddir/src/fe-gtk/birdchat --no-plugins 2>&1 | tee debug.log

# Check for core dump
if [ -f core ]; then
    echo ""
    echo "💥 CORE DUMP FOUND! Analyzing with gdb..."
    gdb --batch --ex run --ex bt --ex quit ./builddir/src/fe-gtk/birdchat core
elif [ $? -ne 0 ]; then
    echo ""
    echo "⚠️  Application exited with non-zero status: $?"
    echo "Check debug.log for details"
else
    echo ""
    echo "✅ Application exited normally"
fi

echo ""
echo "Debug output saved to: debug.log"