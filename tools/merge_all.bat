@echo off
setlocal enabledelayedexpansion

:: Check if merge_sro_files.py exists locally in same directory
set "PYTHON_SCRIPT=%~dp0merge_sro_files.py"
set "PYTHON_SPLIT_SCRIPT=%~dp0split_sro_files.py"
set "TEMP_COPY=0"
set "TEMP_SPLIT_COPY=0"

if not exist "%PYTHON_SCRIPT%" (
    set "SOURCE_SCRIPT=c:\Users\alpka\Desktop\Game\sro_extensions\tools\merge_sro_files.py"
    if exist "!SOURCE_SCRIPT!" (
        copy "!SOURCE_SCRIPT!" "%~dp0" >nul
        set "TEMP_COPY=1"
    ) else (
        echo Error: merge_sro_files.py not found locally or at source path.
        pause
        exit /b 1
    )
)

if not exist "%PYTHON_SPLIT_SCRIPT%" (
    set "SOURCE_SPLIT_SCRIPT=c:\Users\alpka\Desktop\Game\sro_extensions\tools\split_sro_files.py"
    if exist "!SOURCE_SPLIT_SCRIPT!" (
        copy "!SOURCE_SPLIT_SCRIPT!" "%~dp0" >nul
        set "TEMP_SPLIT_COPY=1"
    )
)

:: Check if python is available
where python >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Python is not installed or not added to your PATH environment variable.
    pause
    exit /b 1
)

:: If a directory is dragged and dropped onto the bat file
if not "%~1"=="" (
    if exist "%~1\" (
        echo Scanning dropped directory: %1
        python "%PYTHON_SCRIPT%" -d %1 --overwrite --delete
    ) else (
        echo Scanning dropped file/index: %1
        python "%PYTHON_SCRIPT%" -i %1 --overwrite --delete
    )
    goto end
)

:: Otherwise, ask the user what they want to do
echo ===================================================
echo           SRO Split / Merge Files Utility
echo ===================================================
echo.
echo [1] Merge all split files in a folder
echo [2] Merge a specific index file (e.g. skilldata.txt)
echo [3] Split a merged file back into sub-files (Smart Split)
echo.
set "choice=1"
set /p choice="Enter choice (1-3, default: 1): "

if "%choice%"=="1" (
    set "target_dir=."
    echo Opening folder selection dialog for Target Folder...
    for /f "delims=" %%I in ('powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.ValidateNames = $false; $f.CheckFileExists = $false; $f.CheckPathExists = $true; $f.FileName = 'Select Folder'; $f.Title = 'Navigate into the Target folder and click Open'; if ($f.ShowDialog() -eq 'OK') { [System.IO.Path]::GetDirectoryName($f.FileName) }"') do set "target_dir=%%I"
    
    set "output_dir="
    echo Opening folder selection dialog for Output Folder...
    for /f "delims=" %%I in ('powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.ValidateNames = $false; $f.CheckFileExists = $false; $f.CheckPathExists = $true; $f.FileName = 'Select Folder'; $f.Title = 'Navigate into the Output folder and click Open (or Cancel to merge in-place)'; if ($f.ShowDialog() -eq 'OK') { [System.IO.Path]::GetDirectoryName($f.FileName) }"') do set "output_dir=%%I"
    
    echo.
    echo Selected Target: !target_dir!
    echo Selected Output: !output_dir!
    echo.
    echo Merging folder...
    if "!output_dir!"=="" (
        python "%PYTHON_SCRIPT%" -d "!target_dir!" --overwrite --delete
    ) else (
        python "%PYTHON_SCRIPT%" -d "!target_dir!" --out-dir "!output_dir!" --delete
    )
) else if "%choice%"=="2" (
    set "index_file="
    echo Opening file selection dialog for SRO Index File...
    for /f "delims=" %%I in ('powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.Filter = 'Text Files (*.txt)|*.txt|All Files (*.*)|*.*'; $f.Title = 'Select SRO Index File'; if ($f.ShowDialog() -eq 'OK') { $f.FileName }"') do set "index_file=%%I"
    
    if "!index_file!"=="" (
        echo No file selected. Exiting.
        goto end
    )
    
    set "output_file="
    echo Opening file save dialog for Merged Output File...
    for /f "delims=" %%I in ('powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.SaveFileDialog; $f.Filter = 'Text Files (*.txt)|*.txt|All Files (*.*)|*.*'; $f.Title = 'Select Merged Output File (Cancel to overwrite in-place)'; if ($f.ShowDialog() -eq 'OK') { $f.FileName }"') do set "output_file=%%I"
    
    echo.
    echo Selected Index: !index_file!
    echo Selected Output: !output_file!
    echo.
    echo Merging file...
    if "!output_file!"=="" (
        python "%PYTHON_SCRIPT%" -i "!index_file!" --overwrite --delete
    ) else (
        python "%PYTHON_SCRIPT%" -i "!index_file!" -o "!output_file!" --delete
    )
) else if "%choice%"=="3" (
    set "merged_file="
    echo Opening file selection dialog for Merged SRO File...
    for /f "delims=" %%I in ('powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.Filter = 'Text Files (*.txt)|*.txt|All Files (*.*)|*.*'; $f.Title = 'Select Merged SRO File (e.g. skilldata.txt)'; if ($f.ShowDialog() -eq 'OK') { $f.FileName }"') do set "merged_file=%%I"
    
    if "!merged_file!"=="" (
        echo No file selected. Exiting.
        goto end
    )
    
    set "backup_file="
    echo Opening file selection dialog for Index Backup File...
    for /f "delims=" %%I in ('powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.Filter = 'Text Files (*.txt)|*.txt|All Files (*.*)|*.*'; $f.Title = 'Select Index Backup File (e.g. skilldata_index_backup.txt)'; if ($f.ShowDialog() -eq 'OK') { $f.FileName }"') do set "backup_file=%%I"
    
    if "!backup_file!"=="" (
        echo No backup file selected. Exiting.
        goto end
    )
    
    set "split_out_dir="
    echo Opening folder selection dialog for Output Folder...
    for /f "delims=" %%I in ('powershell -NoProfile -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.ValidateNames = $false; $f.CheckFileExists = $false; $f.CheckPathExists = $true; $f.FileName = 'Select Folder'; $f.Title = 'Navigate into the Output folder and click Open (or Cancel to save in same folder)'; if ($f.ShowDialog() -eq 'OK') { [System.IO.Path]::GetDirectoryName($f.FileName) }"') do set "split_out_dir=%%I"
    
    echo.
    echo Selected Merged: !merged_file!
    echo Selected Backup: !backup_file!
    echo Selected Output: !split_out_dir!
    echo.
    echo Splitting file...
    if "!split_out_dir!"=="" (
        python "%PYTHON_SPLIT_SCRIPT%" -m "!merged_file!" -b "!backup_file!"
    ) else (
        python "%PYTHON_SPLIT_SCRIPT%" -m "!merged_file!" -b "!backup_file!" -o "!split_out_dir!"
    )
) else (
    echo Invalid choice. Exiting.
)

:end
if "%TEMP_COPY%"=="1" (
    del "%~dp0merge_sro_files.py" >nul
)
if "%TEMP_SPLIT_COPY%"=="1" (
    del "%~dp0split_sro_files.py" >nul
)
echo.
echo Done.
pause
