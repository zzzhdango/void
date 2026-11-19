@echo off
echo ===================================================
echo Building Experimental Space Echo VST3 (Windows)...
echo ===================================================
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
echo.
echo Done! Check build\ExperimentalSpaceEcho_artefacts\Release\VST3\
pause
