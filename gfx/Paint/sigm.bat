@echo off
setlocal EnableDelayedExpansion

:: Config
set INPUT=sheet.png
set OUTPUTFOLDER=exported_sprites
set WIDTH=512
set HEIGHT=512
set SCALE=128x128

:: Create output folder
if not exist "%OUTPUTFOLDER%" mkdir "%OUTPUTFOLDER%"

:: Step 1: Crop all 100 tiles
magick %INPUT% -crop %WIDTH%x%HEIGHT% +repage "%OUTPUTFOLDER%\tile_%%d.png"

:: Step 2: Keep every 4th tile and resize it
cd %OUTPUTFOLDER%
set /a i=0
set /a j=0
for %%f in (tile_*.png) do (
    set /a mod=!j! %% 4
    if !mod! == 0 (
        magick "%%f" -resize %SCALE% "!i!.png"
        set /a i+=1
    )
    del "%%f"
    set /a j+=1
)

echo Done! Exported and resized !i! sprites to "%OUTPUTFOLDER%"
pause