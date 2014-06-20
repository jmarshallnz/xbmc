@ECHO OFF

SETLOCAL

SET EXITCODE=0

SET getdepends=true
SET noclean=false
SET addon=
FOR %%b in (%1, %2, %3) DO (
  IF %%b == nodepends (
    SET getdepends=false
  ) ELSE ( IF %%b == noclean (
    SET noclean=true
  ) ELSE ( IF %%b == clean (
    SET noclean=false
  ) ELSE (
    SET addon=%%b
  )))
)

rem set Visual C++ build environment
call "%VS120COMNTOOLS%..\..\VC\bin\vcvars32.bat"

SET WORKDIR=%WORKSPACE%

IF "%WORKDIR%" == "" (
  SET WORKDIR=%CD%\..\..\..
)

rem setup some paths that we need later
SET CUR_PATH=%CD%
SET BASE_PATH=%WORKDIR%\project\cmake
SET ADDONS_PATH=%BASE_PATH%\addons
SET ADDON_DEPENDS_PATH=%ADDONS_PATH%\output
SET ADDONS_BUILD_PATH=%ADDONS_PATH%\build

SET ERRORFILE=%BASE_PATH%\make-addons.error

SET XBMC_INCLUDE_PATH=%ADDON_DEPENDS_PATH%\include\xbmc
SET XBMC_LIB_PATH=%ADDON_DEPENDS_PATH%\lib\xbmc

IF %getdepends% == true (
  ECHO --------------------------------------------------
  ECHO Building addon dependencies
  ECHO --------------------------------------------------
  
  SET addon_depends_mode=clean
  IF %noclean% == true (
    SET addon_depends_mode=noclean
  )

  CALL make-addon-depends.bat %addon_depends_mode%
  IF ERRORLEVEL 1 (
    ECHO make-addon-depends error level: %ERRORLEVEL% > %ERRORFILE%
    GOTO ERROR
  )

  ECHO.
)

rem make sure the xbmc include and library paths exist
IF NOT EXIST "%XBMC_INCLUDE_PATH%" (
  MKDIR "%XBMC_INCLUDE_PATH%"
)
IF NOT EXIST "%XBMC_LIB_PATH%" (
  MKDIR "%XBMC_LIB_PATH%"
)

rem now copy the cmake helper scripts to where they are expected
COPY %BASE_PATH%\xbmc-addon-helpers.cmake %XBMC_LIB_PATH%
COPY %BASE_PATH%\AddOptions.cmake %XBMC_LIB_PATH%

rem go into the addons directory
CD %ADDONS_PATH%

IF %noclean% == false (
  rem remove the build directory if it exists
  IF EXIST "%ADDONS_BUILD_PATH%" (
    RMDIR "%ADDONS_BUILD_PATH%" /S /Q > NUL
  )
)

rem create the build directory
IF NOT EXIST "%ADDONS_BUILD_PATH%" MKDIR "%ADDONS_BUILD_PATH%"

rem go into the build directory
CD "%ADDONS_BUILD_PATH%"

ECHO --------------------------------------------------
ECHO Building addons
ECHO --------------------------------------------------

rem execute cmake to generate makefiles processable by nmake
cmake "%ADDONS_PATH%" -G "NMake Makefiles" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_USER_MAKE_RULES_OVERRIDE="%BASE_PATH%/xbmc-c-flag-overrides.cmake" ^
      -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%BASE_PATH%/xbmc-cxx-flag-overrides.cmake" ^
      -DXBMCROOT=%WORKDIR% ^
      -DPREFIX=%ADDON_DEPENDS_PATH% ^
      -DOUTPUT_DIR=%WORKDIR%\project\Win32BuildSetup\BUILD_WIN32\Xbmc\xbmc-addons ^
      -DPACKAGE_ZIP=1 ^
      -DARCH_DEFINES="-DTARGET_WINDOWS -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_USE_32BIT_TIME_T -D_WINSOCKAPI_" ^
      -DXBMC_MAJOR_VERSION=14 ^
      -DXBMC_MINOR_VERSION=0
IF ERRORLEVEL 1 (
  ECHO cmake error level: %ERRORLEVEL% > %ERRORFILE%
  GOTO ERROR
)

rem execute nmake to build the addons
nmake %addon%
IF ERRORLEVEL 1 (
  ECHO nmake error level: %ERRORLEVEL% > %ERRORFILE%
  GOTO ERROR
)

rem everything was fine
GOTO END

:ERROR
rem something went wrong
ECHO Failed to build addons
ECHO See %ERRORFILE% for more details
SET EXITCODE=1

:END
rem go back to the original directory
cd %CUR_PATH%

rem exit the script with the defined exitcode
EXIT /B %EXITCODE%