@echo off

if "%~1"=="--all" (
    for %%f in (input\*.log) do (
        cmswlogparser.exe -i "%%f" --o output
    )
) else (
   echo USAGE: rn.bat [--all]
   echo Make sure that all the log files are in the `input/` folder [there shall be at least one .log file]
)
