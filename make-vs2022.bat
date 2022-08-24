@set "CURRENT_PATH=%~dp0"
@pushd "%CURRENT_PATH%"

call "./tools/premake5.exe" --file=project.lua --os=windows --cc=clang vs2022

@popd