@echo off
setlocal EnableDelayedExpansion

:: Input and output configuration
set INPUT=sheet.png
set OUTPUTFOLDER=exported_sprites
set TEMP=__temp_tiles
set WIDTH=512
set HEIGHT=512
set SCALE=128x128

:: Create folders
if not exist "%TEMP%" mkdir "%TEMP%"
if not exist "%OUTPUTFOLDER%" mkdir "%OUTPUTFOLDER%"

:: Step 1: Crop into 512x512 tiles with zero-padded names
magick %INPUT% -crop %WIDTH%x%HEIGHT% +repage "%TEMP%\tile_%%03d.png"

:: Step 2: Process every 4th tile
set /a i=0
for /l %%n in (0,4,96) do (
    set "num=00%%n"
    set "num=!num:~-3!"
    magick "%TEMP%\tile_!num!.png" -resize %SCALE% "%OUTPUTFOLDER%\!i!.png"
    set /a i+=1
)

:: Step 3: Clean up
rmdir /s /q "%TEMP%"

echo Done! Exported !i! resized frames (every 4th) to "%OUTPUTFOLDER%"
pause