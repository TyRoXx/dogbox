pushd vcpkg || exit /B 1
::bootstrap-vcpkg.bat
set /p vcpkg_dependencies=<..\vcpkg_dependencies
vcpkg install --triplet x64-windows %vcpkg_dependencies% || exit /B 1
