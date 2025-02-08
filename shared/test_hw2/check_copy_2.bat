@echo off

REM Get the script name
set "file=%~nx0"

REM Extract the number from the original script name
set "num="
for /f "tokens=3 delims=_." %%A in ("%file%") do (
    set "num=%%~A"
)

REM Generate the command
set "command=java -jar copy_check/jplag-2.11.8-SNAPSHOT-jar-with-dependencies.jar -m 60%% -l C/C++ -s ../all_subs/HW%num%-n"

REM Display the command
echo Running command: %command%

REM Uncomment the line below to execute the command
%command%
