
:: Build script for rc_genicam_3dviewer under Windows

@echo off
setlocal enabledelayedexpansion

where nmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo This must be run in Visual Studio command prompt for x64
  exit /b 1
)

where git >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo You must download and install git from: git-scm.com/download/win
  exit /b 1
)

where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo You must download and install cmake from: https://cmake.org/download/
  exit /b 1
)

:: Create directories building and installing

if not exist "build\" mkdir build
cd build

if not exist "install\" mkdir install
cd install
set INSTALL_PATH=%CD%

if not exist "bin\" mkdir bin
if not exist "include\" mkdir include
if not exist "include\GL\" mkdir include\GL
if not exist "lib" mkdir lib

set LIB=%LIB%;%INSTALL_PATH%\lib

cd ..\..\..

:: Clone all missing repositories

if not exist "FreeGLUT\" (
  git clone https://github.com/dcnieho/FreeGLUT.git
  git checkout 349a23dcc1264a76deb79962d1c90462ad0c6f50
)

if not exist "glew-2.2.0\" (
  echo You must download and unpack https://sourceforge.net/projects/glew/files/glew/2.2.0/glew-2.2.0-win32.zip/download
  exit /b 1
)

if not exist "%INSTALL_PATH%\include\GL\glew.h" (
  copy glew-2.2.0\include\GL\* %INSTALL_PATH%\include\GL
)

if not exist "%INSTALL_PATH%\bin\glew32.dll" (
  copy glew-2.2.0\bin\Release\x64\glew32.dll %INSTALL_PATH%\bin
)

if not exist "%INSTALL_PATH%\lib\glew32.lib" (
  copy glew-2.2.0\lib\Release\x64\glew32.lib %INSTALL_PATH%\lib
)

set OPT_GLEW=-DGLEW_INCLUDE_DIR="%CD%\glew-2.2.0\include" -DGLEW_SHARED_LIBRARY_RELEASE="%CD%\glew-2.2.0\lib\Release\x64\glew32.lib"

if not exist "cvkit\" (
  git clone https://github.com/roboception/cvkit.git
)

if not exist "rc_genicam_api\" (
  git clone https://github.com/roboception/rc_genicam_api.git
)

echo ----- Building FreeGLUT -----

cd FreeGLUT/freeglut/freeglut

if not exist "build\" mkdir build
cd build

if exist "build_rc_genicam_3dviewer\" (
  cd build_rc_genicam_3dviewer\
) else (
  mkdir build_rc_genicam_3dviewer\
  cd build_rc_genicam_3dviewer\
  cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%INSTALL_PATH%" ..\..
)

nmake install
if %ERRORLEVEL% NEQ 0 exit /b 1

cp "%INSTALL_PATH%\include\GL\freeglut.h" "%INSTALL_PATH%\include\GL\glut.h"
set OPT_GLUT=-DGLUT_INCLUDE_DIR="%INSTALL_PATH%\include" -DGLUT_glut_LIBRARY_RELEASE="%INSTALL_PATH%\lib\freeglut.lib"

cd ..\..\..\..\..

echo ----- Building cvkit -----

cd cvkit

if not exist "build\" mkdir build
cd build

if exist "build_rc_genicam_3dviewer\" (
  cd build_rc_genicam_3dviewer\
) else (
  mkdir build_rc_genicam_3dviewer\
  cd build_rc_genicam_3dviewer\
  cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%INSTALL_PATH%" %OPT_GLUT% %OPT_GLEW% ..\..
)

nmake install
if %ERRORLEVEL% NEQ 0 exit /b 1

cd ..\..\..

:: General options for all cmake packages

set OPT=-DUSE_SSE2=0 -DUSE_SSE4.2=0 -DUSE_AVX=0 -DUSE_AVX2=0 -DCMAKE_BUILD_TYPE=Release

echo ----- Building rc_genicam_api -----

cd rc_genicam_api

if not exist "build\" mkdir build
cd build

if exist "build_rc_genicam_3dviewer\" (
  cd build_rc_genicam_3dviewer\
) else (
  mkdir build_rc_genicam_3dviewer\
  cd build_rc_genicam_3dviewer\
  cmake -G "NMake Makefiles" %OPT% -DCMAKE_INSTALL_PREFIX="%INSTALL_PATH%" ..\..
)

nmake install
if %ERRORLEVEL% NEQ 0 exit /b 1

cd ..\..\..

echo ----- Building rc_genicam_3dviewer -----

cd rc_genicam_3dviewer\build

if exist "build_rc_genicam_3dviewer\" (
  cd build_rc_genicam_3dviewer\
) else (
  mkdir build_rc_genicam_3dviewer\
  cd build_rc_genicam_3dviewer\
  cmake -G "NMake Makefiles" %OPT% -DCMAKE_INSTALL_PREFIX="%INSTALL_PATH%" ..\..
)

nmake install
if %ERRORLEVEL% NEQ 0 exit /b 1

cd ..

echo ----- Extracting files for publication -----

for /F "tokens=* USEBACKQ" %%F in (`git describe`) do (set VERSION=%%F)

set TARGET=rc_genicam_3dviewer-%VERSION%-win64
if not exist "%TARGET%\" mkdir %TARGET%

copy %INSTALL_PATH%\bin\GCBase_*.dll %TARGET%
copy %INSTALL_PATH%\bin\GenApi_*.dll %TARGET%
copy %INSTALL_PATH%\bin\Log_*.dll %TARGET%
copy %INSTALL_PATH%\bin\log4cpp_*.dll %TARGET%
copy %INSTALL_PATH%\bin\MathParser_*.dll %TARGET%
copy %INSTALL_PATH%\bin\NodeMapData_*.dll %TARGET%
copy %INSTALL_PATH%\bin\XmlParser_*.dll %TARGET%
copy %INSTALL_PATH%\bin\msvcp140.dll %TARGET%
copy %INSTALL_PATH%\bin\vcruntime140.dll %TARGET%
copy %INSTALL_PATH%\bin\rc_genicam_api.dll %TARGET%

if not exist "%TARGET%\rc_genicam_api\" mkdir %TARGET%\rc_genicam_api

copy %INSTALL_PATH%\bin\bgapi2_gige.cti %TARGET%\rc_genicam_api
copy %INSTALL_PATH%\bin\bgapi2_usb.cti %TARGET%\rc_genicam_api
copy %INSTALL_PATH%\bin\bsysgige.xml %TARGET%\rc_genicam_api
copy %INSTALL_PATH%\bin\bsysusb.xml %TARGET%\rc_genicam_api

copy %INSTALL_PATH%\bin\gutil.dll %TARGET%
copy %INSTALL_PATH%\bin\bgui.dll %TARGET%
copy %INSTALL_PATH%\bin\gimage.dll %TARGET%
copy %INSTALL_PATH%\bin\gmath.dll %TARGET%
copy %INSTALL_PATH%\bin\gvr.dll %TARGET%
copy %INSTALL_PATH%\bin\freeglut.dll %TARGET%
copy %INSTALL_PATH%\bin\glew32.dll %TARGET%
copy %INSTALL_PATH%\bin\gc_3dviewer.exe %TARGET%

