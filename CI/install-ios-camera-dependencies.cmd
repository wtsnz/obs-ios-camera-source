REM if not exist %OpenSSLBaseDir% (
REM 	curl -kLO https://curl.se/windows/dl-7.75.0/openssl-1.1.1i-win32-mingw.zip -f --retry 5 -z openssl-1.1.1i-win32-mingw.zip
REM     7z x openssl-1.1.1i-win32-mingw.zip -o%OpenSSLBaseDir%

REM     curl -kLO https://curl.se/windows/dl-7.75.0/openssl-1.1.1i-win64-mingw.zip -f --retry 5 -z openssl-1.1.1i-win64-mingw.zip
REM     7z x openssl-1.1.1i-win64-mingw.zip -o%OpenSSLBaseDir%
REM ) else (
REM 	echo "OpenSSL is already installed. Download skipped."
REM )

REM dir %OpenSSLBaseDir%

cd c:\vcpkg
vcpkg integrate install

vcpkg install dirent:x86-windows-static
vcpkg install dirent:x64-windows-static
vcpkg install openssl:x86-windows-static
vcpkg install openssl:x64-windows-static