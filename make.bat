@echo off

IF not exist build/ ( 
	rmdir /S /q %~dp0\build
	mkdir %~dp0\build
	cd %~dp0\build
	cmake ../.
	cd ..
)

call "%VS110COMNTOOLS%vsvars32.bat"
msbuild build/INSTALL.vcxproj