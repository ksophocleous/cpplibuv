@echo off
rmdir /S /q build
mkdir build
cd build
cmake ../.
cd ..

msbuild build/INSTALL.vcxproj