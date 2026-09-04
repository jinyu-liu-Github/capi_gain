# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.14.3)

project(capi_gain C CXX ASM)

enable_language(ASM)

add_definitions(-DCAPI_SO_VERSION="capi_gain.so.1.0")
#adding
if(HEXAGON_SDK_ROOT)
    include(${HEXAGON_SDK_ROOT}/build/cmake/hexagon_fun.cmake)
else()
    include(${HEXAGON_CMAKE_ROOT}/hexagon_fun.cmake)
endif()

# Qualcomm Hexagon Libraries (QHL) prebuilt for the active tool/architecture.
set(QHL_ROOT "${HEXAGON_SDK_ROOT}/libs/qhl")
set(QHL_PREBUILT_DIR "${QHL_ROOT}/prebuilt/${PREBUILT_LIB_DIR}")
set(QHL_LIBS)

foreach(QHL_LIB IN ITEMS qhdsp qhcomplex qhmath qhblas)
    add_library(${QHL_LIB}_prebuilt STATIC IMPORTED)
    set_target_properties(${QHL_LIB}_prebuilt PROPERTIES
        IMPORTED_LOCATION "${QHL_PREBUILT_DIR}/lib${QHL_LIB}.a"
    )
    list(APPEND QHL_LIBS ${QHL_LIB}_prebuilt)
endforeach()
#
set(CMAKE_DEBUG_TARGET_PROPERTIES
        INCLUDE_DIRECTORIES
        COMPILE_DEFINITIONS
        POSITION_INDEPENDENT_CODE
        CONTAINER_SIZE_REQUIRED
        LIB_VERSION
    )

#Dependent library Paths
set(test_main_src ${HEXAGON_SDK_ROOT}/utils/test_main)
set(test_main_inc ${HEXAGON_SDK_ROOT}/utils/test_main/inc)
set(test_profile_src ${HEXAGON_SDK_ROOT}/utils/sim_profiler)
set(test_profile_inc ${HEXAGON_SDK_ROOT}/utils/sim_profiler/inc)
set(test_utils_src ${HEXAGON_SDK_ROOT}/utils/sim_utils)

set(test_capi_src ${HEXAGON_SDK_ROOT}/addons/audio/spf/test/test_capi)
set(test_capi_inc ${HEXAGON_SDK_ROOT}/addons/audio/spf/test/test_capi/inc)
set(spf_ref_counter_src ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/utils/cmn)
set(spf_ref_counter_inc ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/utils/cmn/inc)
set(spf_list_utils_src ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/utils/list)
set(spf_list_utils_inc ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/utils/list/inc)	

set(capi_utils_props_src ${HEXAGON_SDK_ROOT}/addons/audio/spf/libs/capi_utils_props)
set(capi_utils_props_inc ${HEXAGON_SDK_ROOT}/addons/audio/spf/libs/capi_utils_props/inc)	
set(posal_utils_src ${HEXAGON_SDK_ROOT}/addons/audio/spf/libs/posal_utils)
set(posal_utils_inc ${HEXAGON_SDK_ROOT}/addons/audio/spf/libs/posal_utils/inc)	
set(posal_stub_inc ${HEXAGON_SDK_ROOT}/addons/audio/spf/libs/posal_stub/inc)
set(posal_stub_src ${HEXAGON_SDK_ROOT}/addons/audio/spf/libs/posal_stub/)


#include(${HEXAGON_CMAKE_ROOT}/hexagon_fun.cmake)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
set(NO_QURT_INC 1)

#set(EXE_LD_FLAGS)
#list2string(EXE_LD_FLAGS
#    -march=hexagon
#    -mcpu=hexagon${V_ARCH}
#    "-o <TARGET>"
#    -G0
#    -Wl,-L${HEXAGON_TOOLCHAIN}/Tools/target/hexagon/lib/${HEXAGON_ARCH}/G0/
#    -Wl,-L${HEXAGON_TOOLCHAIN}/Tools/target/hexagon/lib/${HEXAGON_ARCH}/
#    -Wl,-L${HEXAGON_TOOLCHAIN}/Tools/target/hexagon/lib/
#    -Wl,--no-threads -Wl,--dynamic-linker= -Wl,-E -Wl,--force-dynamic,-u,main
#    -Wl,--start-group
#    "<OBJECTS>"
#    "<LINK_LIBRARIES>"
#    ${HEXAGON_TOOLCHAIN}/Tools/target/hexagon/lib/${HEXAGON_ARCH}/G0/libhexagon.a
#    -Wl,--end-group
#    -Wl,--start-group -lstandalone -lc -lgcc -Wl,--end-group
#    )
STRING(REPLACE ";" " " EXE_LD_FLAGS "${EXE_LD_FLAGS}")


set(HEXAGON_C_LINK_EXECUTABLE_LINK_OPTIONS "${EXE_LD_FLAGS}" )
set(HEXAGON_CXX_LINK_EXECUTABLE_LINK_OPTIONS "${EXE_LD_FLAGS}")
set(CMAKE_C_LINK_EXECUTABLE "${HEXAGON_LINKER} ${HEXAGON_C_LINK_EXECUTABLE_LINK_OPTIONS}")
set(CMAKE_CXX_LINK_EXECUTABLE "${HEXAGON_LINKER} ${HEXAGON_CXX_LINK_EXECUTABLE_LINK_OPTIONS}")



#################################################
#Hexagon Dynamic Library
#################################################
#if(${BUILD_NAME} MATCHES "_q")

    #################################################
    # Static elf
    #################################################

    set(incs
        ${CMAKE_CURRENT_SOURCE_DIR}/api/
        ${CMAKE_CURRENT_SOURCE_DIR}/capi/inc/
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/inc/
        ${QHL_ROOT}/inc/qhmath/
        ${QHL_ROOT}/inc/qhcomplex/
        ${QHL_ROOT}/inc/qhblas/
        ${QHL_ROOT}/inc/qhdsp/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/test/test_capi/inc/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/api/ar_utils/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/api/apm/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/interfaces/module/capi/adv/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/interfaces/module/capi/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/interfaces/module/shared_lib_api/inc/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/platform/posal/inc/hexagon/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/platform/posal/inc/generic/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/api/ar_utils/hexagon/sim/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/spf/interfaces/module/metadata/api/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/api/modules/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/platform/posal/inc/
	${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/gpr/platform/qurt/osal/api/
        ${HEXAGON_SDK_ROOT}/addons/audio/spf/incs/core/api/services/

     )

    
    add_executable(capi_gain_q
        ${CMAKE_CURRENT_SOURCE_DIR}/../../test/test_capi/src/capi_unit_test
        ${CMAKE_CURRENT_SOURCE_DIR}/capi_entry_functions
        )

    add_library(capi_gain
            SHARED
            ${CMAKE_CURRENT_SOURCE_DIR}/capi/src/capi_gain
            ${CMAKE_CURRENT_SOURCE_DIR}/capi/src/capi_gain_utils
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/src/gain_lib
            ${CMAKE_CURRENT_SOURCE_DIR}/../so_version/version_shared_lib
        )

    add_library(capi_gain_static
            STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/capi/src/capi_gain
            ${CMAKE_CURRENT_SOURCE_DIR}/capi/src/capi_gain_utils
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/src/gain_lib
            ${CMAKE_CURRENT_SOURCE_DIR}/../so_version/version_shared_lib
        )


    # Set some of the flags
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions ")

    ##############################
    #
    # prepare_libraries_hexagon (<hexagon_targets> <hexagon_incs>  <hexagon_libs>
    #   libName [libNames...])
    #
    # The first 3 arguments will be the output arguments.  And the
    # following arguments will be the library names.  Without surfix, it is
    # treated as a dynamic library. surfixed with ".a" will be processed as
    # static library. And surffixed with ".so" will be processed as dynamic
    # library.
    #
    # This function will do the following:
    #
    # (1) For all libraries that specified, it will search Hexagon SDK tree
    # to find the corresponding library, and add a target into the
    # <hexagon_targets> list.  The custom_target will specify what to do
    # for that target.  It can be going into that corresponding directory to
    # build the directory or do nothing if it's prebuilt library. Caller of
    # this function can add this target as the dependency of their own
    # target or multiple targets
    #
    # (2) This call will add the "library specific" include directories
    # into <hexagon_incs> list. Caller of the function can add this include
    # path list into their corresponding include list
    #
    # (3) This library call will also return a "library-target" list
    # <hexagon_libs> so that it can be added into the linker dependency
    # list from target_link_libraries call
    #
    ##############################
    link_custom_library(capi_gain_q test_util)
    link_custom_library(capi_gain_q atomic)
    link_custom_library(capi_gain_q rtld)
    set(TOOLS_HEXAGON_TARGET_DIR "${HEXAGON_TOOLCHAIN}/Tools/target/hexagon/lib/${DSP_VERSION}/")
    target_link_libraries(capi_gain_q ${TOOLS_HEXAGON_TARGET_DIR}/libhexagon.a)
    prepare_libraries_hexagon(hexagonTargets hexagonIncs hexagonLibs hexagonSharedLibs
        test_main.a 
        test_profile.a
        test_capi.a
        posal_utils.a
        spf_ref_counter.a
        spf_list_utils.a
	posal_stub.a
    )

    prepare_libraries_hexagon(hexagonCommonTargets hexagonCommonIncs hexagonCommonLibs hexagonCommonSharedLibs
        capi_utils_props.a
    )
	message(STATUS "${hexagonCommonTargets}")
    #Import the Required RTLD Lib
#    add_library(rtld STATIC IMPORTED) # or STATIC instead of SHARED
#    set_target_properties(rtld PROPERTIES
#        IMPORTED_LOCATION "${HEXAGON_SDK_ROOT}/ipc/fastrpc/rtld/ship/${PREBUILT_LIB_DIR}/rtld.a"
#        INTERFACE_INCLUDE_DIRECTORIES "${HEXAGON_SDK_ROOT}/ipc/fastrpc/rtld/ship/inc/"
#    )

    #Import the Required libstdc++ Lib
    #add_library(libstdc++ STATIC IMPORTED) # or STATIC instead of SHARED
    #set_target_properties(libstdc++ PROPERTIES
    #    IMPORTED_LOCATION "${HEXAGON_SDK_ROOT}/tools/HEXAGON_Tools/8.4.04/Tools/target/hexagon/lib/v68/G0/libstdc++.a"
    #)

    #Import the Required voice_imc_utils Lib
    #add_library(voice_imc_utils STATIC IMPORTED) # or STATIC instead of SHARED
    #set_target_properties(voice_imc_utils PROPERTIES
    #    IMPORTED_LOCATION "${HEXAGON_SDK_ROOT}/addons/audio/libs/voice_imc_utils/ship/${PREBUILT_LIB_DIR}/voice_imc_utils.a"
    #)

    #Include necessary dependent lib directory
    include_directories(
        ${incs}
        ${hexagonIncs}
        ${hexagonCommonIncs}
    )

    #Add hexagonTargets as one of your dependencies
    add_dependencies(capi_gain ${hexagonCommonTargets})
    add_dependencies(capi_gain_q capi_gain_static ${hexagonTargets} ${hexagonCommonTargets})

    ##Add hexagonLibs as your target link dependency
    target_link_libraries(capi_gain ${QHL_LIBS} ${hexagonCommonLibs})
    target_link_libraries(capi_gain_static ${QHL_LIBS})
    target_link_libraries(capi_gain_q capi_gain_static ${hexagonLibs} ${hexagonCommonLibs})
    #target_link_libraries(capi_gain_q ${hexagonLibs} ${hexagonCommonLibs})    

    set(HEXAGON_EXEC capi_gain_q)
    set(HEXAGON_EXEC_CMD_OPTIONS ${HEXAGON_EXEC_CMD_OPTIONS} -f epsilon -w 256 -h 64)

    copy_binaries(capi_gain_q capi_gain_static capi_gain)
#endif()

