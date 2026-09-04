source ${HEXAGON_SDK_ROOT}/build/cmake/cmake_configure.bash

export PATH="${HEXAGON_SDK_ROOT}\\tools\\utils\\gow-0.8.0\\bin\\:$PATH"
export PATH="C:\\WINDOWS\\system32\\:$PATH"

# invoke command to construct Build Tag and Build the required variant
doBuild "$@" TEST_NAME=capi_gain
if [ $CLEAN_ONLY -eq  1 ];
then
    echo "Cleaning the build Directories"
    doClean rpcmem atomic sim_util
else
    cd ${_src} && \
      ${CMAKE} \
      --build \
      ${_build} \
      --clean-first \
      --target ${BUILD_TARGET_NAME} -- \
      && \
      cd -
fi

