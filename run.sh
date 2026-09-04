#!/bin/sh

TOOL=$1
ARCH=$2
DEFAULT_ARCH=v73
DEFAULT_TOOL=toolv88
TOOL_RESULT=0
ARCH_RESULT=0

common_run()
{
    echo "Running with TOOLS_VARIANT=$TOOL and DSP_ARCH=$ARCH"
    echo "======== Running windows test command for CAPI_GAIN ========="
    echo $DEFAULT_HEXAGON_TOOLS_ROOT
    VARIANT=hexagon_Debug_$TOOL\_$ARCH
    $DEFAULT_HEXAGON_TOOLS_ROOT/Tools/bin/hexagon-sim -m$ARCH --simulated_returnval --usefs $VARIANT --pmu_statsfile $VARIANT/pmu_stats.txt $VARIANT/capi_gain_q -- -i data/input/chirp16_2_48000Hz_new.raw -o data/op.pcm -c data/cfg/capiv2_gain_48k_2ch_gain_change.cfg
    echo "======== End windows test command for CAPI_GAIN ========="
}

help_exit()
{
    echo "Usage:"
    echo "Example 1 default : run.sh"
    echo "Example 2 Run with hexagon tool version 8.4 : run.sh toolv84"
    echo "Example 3 Run with hexagon tool version 8.5 with arch v68 : run.sh toolv84 v68"
}

echo "*********************************************************"
echo "******** IF ANY FAILURE, run help : run.sh help *********"
echo "*********************************************************"

if [ $# -eq 0 ]
then
    echo "No parameters, trying default run."
    echo "####################################################"
    echo "##### If default fails, try run.sh help ####"
    echo "####################################################"
    TOOL=$DEFAULT_TOOL
    ARCH=$DEFAULT_ARCH
    common_run
    exit 0
else
    echo "Parameters: $@"
fi

if [ $1 = help ]
then
    help_exit
    exit 0
fi

if [ $TOOL = toolv82 ] || [ $TOOL = toolv83 ] || [ $TOOL = toolv84 ] || [ $TOOL = toolv85 ] || [ $TOOL = toolv86 ] || [ $TOOL = toolv87 ] || [ $TOOL = toolv88 ]
then
    TOOL_RESULT=1
fi

if [ $TOOL_RESULT -eq 0 ]
then     	
    echo "Incorrect tool version $TOOL, exiting"
    help_exit
    exit 0
fi

if [ $# -eq 1 ]
then	
    echo "No arch version set, trying default run"
    ARCH=$DEFAULT_ARCH
fi

if [ $ARCH = v65 ] || [ $ARCH = v66 ] || [ $ARCH = v68 ] || [ $ARCH = v69 ] || [ $ARCH = v73 ] || [ $ARCH = v79 ] || [ $ARCH = v81 ]
then
    ARCH_RESULT=1
fi

if [ $ARCH_RESULT -eq 0 ]
then	
    echo "Incorrect arch version $ARCH, exiting"
    help_exit
    exit 0
fi

common_run
