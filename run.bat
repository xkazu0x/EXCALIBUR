@echo off

if not exist build\excalibur.exe call build

pushd build
call excalibur.exe
popd
