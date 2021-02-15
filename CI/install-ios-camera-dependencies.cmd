cd c:\vcpkg
vcpkg integrate install

vcpkg install dirent:x86-windows-static
vcpkg install dirent:x64-windows-static
vcpkg install openssl:x86-windows-static
vcpkg install openssl:x64-windows-static