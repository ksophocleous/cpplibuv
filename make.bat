@echo off

IF not exist build/ ( 
	rmdir /S /q %~dp0\build
	mkdir %~dp0\build
	cd %~dp0\build
	cmake ../.
	cd ..
)

msbuild build/INSTALL.vcxproj