@echo off

rd /s /q build

mkdir build

cd build

cmake -G"Visual Studio 17 2022" -A win32 ..

cd ..	

pause
