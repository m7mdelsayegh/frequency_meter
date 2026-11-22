@echo off
REM ==========================================================================
REM  build_android.bat  -  Command-line build of the Frequency Meter Android app
REM
REM  Requires: RAD Studio / Delphi 12 (Studio 23.0) with the Android SDK/NDK
REM            installed through the RAD Studio Platform Manager.
REM
REM  Unlike the old "autofix_and_build.bat", this script does NOT overwrite
REM  freq_meter.pas / freq_meter.fmx. It only builds what is in this folder.
REM ==========================================================================
setlocal
cd /d "%~dp0"

set "RSVARS=C:\Program Files (x86)\Embarcadero\Studio\23.0\bin\rsvars.bat"
if not exist "%RSVARS%" (
  echo ERROR: rsvars.bat not found at "%RSVARS%"
  echo Edit the RSVARS line in this script to match your RAD Studio version.
  goto :fail
)
call "%RSVARS%" || goto :fail

echo.
echo === 1/3 Compiling Android 64-bit ===
msbuild "freq_m.dproj" /t:Build /p:Config=Debug /p:Platform=Android64 /v:minimal || goto :fail

echo.
echo === 2/3 Packaging APK ===
REM The msbuild "Deploy" target only exists when freq_m.deployproj is present.
REM RAD Studio creates that file the first time you open Project ^> Deployment
REM (select Android 64-bit, then File ^> Save All). Without it, use the IDE.
if not exist "freq_m.deployproj" (
  echo.
  echo Compile succeeded, but freq_m.deployproj is missing so the APK cannot be
  echo packaged from the command line. Do this ONCE in RAD Studio:
  echo   1. Open freq_m.dproj
  echo   2. Select target platform "Android 64-bit"
  echo   3. Project ^> Deployment  ^(the Deployment Manager opens^)
  echo   4. File ^> Save All   -^> freq_m.deployproj is created
  echo Then run this script again ^(or simply press F9 / Project ^> Deploy in the IDE^).
  goto :fail
)
msbuild "freq_m.dproj" /t:Deploy /p:Config=Debug /p:Platform=Android64 /v:minimal || goto :fail

set "APK=%~dp0Android64\Debug\freq_m\bin\freq_m.apk"
if not exist "%APK%" goto :fail
echo Built APK: %APK%

echo.
echo === 3/3 Installing via ADB (only if adb is on PATH and a phone is connected) ===
where adb >nul 2>nul && (
  adb devices
  adb install -r "%APK%"
)

echo.
echo *** SUCCESS - APK IS BUILT ***
pause
exit /b 0

:fail
echo.
echo *** BUILD FAILED - check the error above ***
pause
exit /b 1
