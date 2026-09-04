@echo off 

set TOOL=%1
set ARCH=%2
set DEFAULT_ARCH=v73
set DEFAULT_TOOL=toolv88
set /A TOOL_RESULT = 0
set /A ARCH_RESULT = 0

echo *********************************************************
echo **** IF ANY FAILURE, run help : sample_make.cmd help ****
echo *********************************************************

if "%~1"=="" (
    echo No parameters, trying default compile.
    echo ####################################################    
    echo #### If default fails, try sample_make.cmd help ####
    echo ####################################################
    set TOOL=%DEFAULT_TOOL%
    set ARCH=%DEFAULT_ARCH%    
    goto :common_compile
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
    echo No arch version set, trying default compile
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

:common_compile
echo  ===== Compiling with TOOLS_VARIANT=%TOOL% and DSP_ARCH=%ARCH% ===== 
echo To Compile with different options try === eg :- sample_make.cmd toolv86 v73 ====
echo  ===== Waiting 4 seconds ===== 
sleep 4
make hexagon BUILD=Debug DSP_ARCH=%ARCH% TOOLS_VARIANT=%TOOL%
exit /b 0

:help_exit 
echo Usage:
echo Example 1 default : sample_make.cmd
echo Example 2 Make with hexagon tool version 8.4 : sample_make.cmd toolv84
echo Example 3 Make with hexagon tool version 8.5 with arch v68 : sample_make.cmd toolv84 v68
exit /b 0

