#!/bin/sh

TOOL=$1
ARCH=$2
DEFAULT_ARCH=v73
DEFAULT_TOOL=toolv88
TOOL_RESULT=0
ARCH_RESULT=0

common_compile()
{
    echo "Compiling with TOOLS_VARIANT=$TOOL and DSP_ARCH=$ARCH"
    make hexagon BUILD=Debug DSP_ARCH=$ARCH TOOLS_VARIANT=$TOOL
}

help_exit()
{
    echo "Usage:"
    echo "Example 1 default : sample_make.sh"
    echo "Example 2 Make with hexagon tool version 8.4 : sample_make.sh toolv84"
    echo "Example 3 Make with hexagon tool version 8.5 with arch v68 : sample_make.sh toolv84 v68"
}

echo "*********************************************************"
echo "**** IF ANY FAILURE, run help : sample_make.sh help *****"
echo "*********************************************************"

if [ $# -eq 0 ]
then
    echo "No parameters, trying default compile."
    echo "####################################################"
    echo "##### If default fails, try sample_make.sh help ####"
    echo "####################################################"
    TOOL=$DEFAULT_TOOL
    ARCH=$DEFAULT_ARCH
    common_compile
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
    echo "No arch version set, trying default compile"
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

common_compile
