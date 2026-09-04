@echo off 

set TOOL=%1
set ARCH=%2
set DEFAULT_ARCH=v73
set DEFAULT_TOOL=toolv88
set /A TOOL_RESULT = 0
set /A ARCH_RESULT = 0

echo *********************************************************
echo **** IF ANY FAILURE, run help : run.bat help ****
echo *********************************************************

if "%~1"=="" (
    echo No parameters, trying default run.
    echo ####################################################    
    echo #### If default fails, try run.bat help ####
    echo ####################################################
    set TOOL=%DEFAULT_TOOL%
    set ARCH=%DEFAULT_ARCH%
    goto :common_run
) else (
    echo Parameters: %*
)

if %1==help goto :help_exit

if %TOOL%==toolv82 set TOOL_RESULT=1
if %TOOL%==toolv83 set TOOL_RESULT=1
if %TOOL%==toolv84 set TOOL_RESULT=1
if %TOOL%==toolv85 set TOOL_RESULT=1
if %TOOL%==toolv86 set TOOL_RESULT=1
if %TOOL%==toolv87 set TOOL_RESULT=1
if %TOOL%==toolv88 set TOOL_RESULT=1

if %TOOL_RESULT%==0 (
    echo Incorrect tool version %TOOL%, exiting
    goto :help_exit
)

if [%2] == [] (
    echo No arch version set, trying default run
    set ARCH=%DEFAULT_ARCH%
)

if %ARCH%==v65 set ARCH_RESULT=1
if %ARCH%==v66 set ARCH_RESULT=1
if %ARCH%==v68 set ARCH_RESULT=1
if %ARCH%==v69 set ARCH_RESULT=1
if %ARCH%==v73 set ARCH_RESULT=1
if %ARCH%==v79 set ARCH_RESULT=1
if %ARCH%==v81 set ARCH_RESULT=1

if %ARCH_RESULT%==0 (
    echo Incorrect arch version %ARCH%, exiting
    goto :help_exit
)

:common_run
echo ===== Run with TOOLS_VARIANT=%TOOL% and DSP_ARCH=%ARCH% ======
echo To Run with different options try === eg :- run.bat toolv86 v73 ====
echo ===== Waiting 4 seconds =====
sleep 4

echo ======== Running windows test command for CAPI_GAIN =========
echo %DEFAULT_HEXAGON_TOOLS_ROOT%
%DEFAULT_HEXAGON_TOOLS_ROOT%\\Tools\bin\hexagon-sim -m%ARCH% --simulated_returnval --usefs hexagon_Debug_%TOOL%_%ARCH% --pmu_statsfile hexagon_Debug_%TOOL%_%ARCH%/pmu_stats.txt hexagon_Debug_%TOOL%_%ARCH%/capi_gain_q -- -i data\input\chirp16_2_48000Hz_new.raw -o data\op.pcm -c data\cfg\capiv2_gain_48k_2ch_gain_change.cfg
echo ======== End windows test command for CAPI_GAIN =========

exit /b 0

:help_exit 
echo Usage:
echo Example 1 default : run.bat
echo Example 2 Run with hexagon tool version 8.4 : run.bat toolv84
echo Example 3 Run with hexagon tool version 8.5 with arch v68 : run.bat toolv84 v68
exit /b 0