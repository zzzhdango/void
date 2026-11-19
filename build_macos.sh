#!/bin/bash
set -e
echo "==================================================="
echo "Building Experimental Space Echo VST3 & AU (macOS)..."
echo "==================================================="
cmake -B build -G Xcode
cmake --build build --config Release
echo "Done! Installed to ~/Library/Audio/Plug-Ins/VST3/ and Components/"
