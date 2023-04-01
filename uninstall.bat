@echo off
echo *****************
echo Author: @rednek46
echo *****************
echo Removing Patch
if exist "%APPDATA%\Spotify\dpapi_bak.dll" (
    del /s /q "%APPDATA%\Spotify\dpapi.dll" > NUL 2>&1
    move "%APPDATA%\Spotify\dpapi_bak.dll" "%APPDATA%\Spotify\dpapi.dll" > NUL 2>&1
) else (
    echo done
)
pause