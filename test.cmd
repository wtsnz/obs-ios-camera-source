git submodule update --init --recursive
cd E:\projects\
if not exist dependencies2015.zip curl -kLO https://obsproject.com/downloads/dependencies2015.zip -f --retry 5 -C -
7z x dependencies2015.zip -odependencies2015
set DepsPath32=%CD%\dependencies2015\win32
set DepsPath64=%CD%\dependencies2015\win64
call E:\projects\obs-websocket\CI\install-setup-qt.cmd
set build_config=Release
call E:\projects\obs-websocket\CI\install-build-obs.cmd
cd E:\projects\obs-websocket\
mkdir build32
mkdir build64
cd ./build32
REM cmake -G "Visual Studio 14 2017" -DQTDIR="%QTDIR32%" -DLibObs_DIR="E:\projects\obs-studio\build32\libobs" -DLIBOBS_INCLUDE_DIR="E:\projects\obs-studio\libobs" -DLIBOBS_LIB="E:\projects\obs-studio\build32\libobs\%build_config%\obs.lib" -DOBS_FRONTEND_LIB="E:\projects\obs-studio\build32\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" ..
cd ../build64
cmake -G "Visual Studio 15 2017 Win64" -DQTDIR="%QTDIR64%" -DLibObs_DIR="E:\projects\obs-studio\build64\libobs" -DLIBOBS_INCLUDE_DIR="E:\projects\obs-studio\libobs" -DLIBOBS_LIB="E:\projects\obs-studio\build64\libobs\%build_config%\obs.lib" -DOBS_FRONTEND_LIB="E:\projects\obs-studio\build64\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" ..
