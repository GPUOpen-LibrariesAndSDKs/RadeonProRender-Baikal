cmake_minimum_required(VERSION 3.8)

project(Baikal CXX)

option(BAIKAL_ENABLE_RAYMASK "Enable visibility flags for shapes (slows down an intersector)" OFF)
option(BAIKAL_ENABLE_DENOISER "Use denoising on output" OFF)
option(BAIKAL_ENABLE_RPR "Enable RadeonProRender API lib" OFF)
option(BAIKAL_ENABLE_UBERV2 "Enable UberV2 support" OFF)
option(BAIKAL_ENABLE_TESTS "Enable tests" ON)
option(BAIKAL_ENABLE_STANDALONE "Enable standalone application build" ON)

#Disabled for now.
#option(BAIKAL_ENABLE_FBX "Enable FBX import" OFF)
#option(BAIKAL_ENABLE_GLTF "Enable glTF import" OFF)
#option(BAIKAL_EMBED_KERNELS "Embed CL kernels into binary module" OFF)

#Sanity checks
if (BAIKAL_ENABLE_GLTF AND NOT BAIKAL_ENABLE_RPR)
    message(FATAL_ERROR "glTF support requires RPR but it's disabled")
endif (BAIKAL_ENABLE_GLTF AND NOT BAIKAL_ENABLE_RPR)

#global settings
if (WIN32)
    add_definitions(/MP -D_CRT_SECURE_NO_WARNINGS)
elseif (UNIX)
    add_definitions(-fvisibility=hidden -Wno-ignored-attributes)
endif (WIN32)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#Find required packages
find_package(Threads REQUIRED)
find_package(OIIO REQUIRED)

if (BAIKAL_ENABLE_STANDALONE OR BAIKAL_ENABLE_RPR)
    find_package(OpenGL REQUIRED)
endif (BAIKAL_ENABLE_STANDALONE OR BAIKAL_ENABLE_RPR)

#Configure RadeonRays build
set(RR_EMBED_KERNELS ON CACHE BOOL "Embed CL kernels into binary module")
set(RR_ALLOW_CPU_DEVICES OFF CACHE BOOL "Allows CPU Devices")
set(RR_USE_OPENCL ON CACHE BOOL "Use OpenCL for GPU hit testing")
set(RR_USE_EMBREE OFF CACHE BOOL "Use Intel(R) Embree for CPU hit testing")
set(RR_USE_VULKAN OFF CACHE BOOL "Use vulkan for GPU hit testing")
set(RR_NO_TESTS ON CACHE BOOL "Don't add any unit tests and remove any test functionality from the library")
set(RR_ENABLE_STATIC OFF CACHE BOOL "Create static libraries rather than dynamic")
set(RR_SHARED_CALC OFF CACHE BOOL "Link Calc(compute abstraction layer) dynamically")
set(RR_ENABLE_RAYMASK $BAIKAL_ENABLE_RAYMASK CACHE BOOL "Enable ray masking in intersection kernels")
set(RR_TUTORIALS OFF CACHE BOOL "Add tutorials projects")
set(RR_SAFE_MATH OFF CACHE BOOL "use safe math")

if (BAIKAL_ENABLE_FBX)
    find_package(FBX_SDK REQUIRED)
endif (BAIKAL_ENABLE_FBX)

if (BAIKAL_ENABLE_STANDALONE OR BAIKAL_ENABLE_RPR)
    find_package(GLEW REQUIRED)
endif (BAIKAL_ENABLE_STANDALONE OR BAIKAL_ENABLE_RPR)

add_subdirectory(RadeonRays)

add_subdirectory(Baikal)

if (BAIKAL_ENABLE_RPR)
    set(RprSupport_HEADER "${Baikal_SOURCE_DIR}/3rdparty/RprSupport/include/")
    add_subdirectory(Rpr)
    add_subdirectory(RprSupport)
endif (BAIKAL_ENABLE_RPR)

if (BAIKAL_ENABLE_STANDALONE)
    find_package(GLFW3 REQUIRED)

    add_subdirectory(BaikalStandalone)
endif (BAIKAL_ENABLE_STANDALONE)

if (BAIKAL_ENABLE_TESTS)
    add_subdirectory(Gtest)
    add_subdirectory(BaikalTest)
    if (BAIKAL_ENABLE_RPR)
        add_subdirectory(RprTest)
    endif (BAIKAL_ENABLE_RPR)
endif (BAIKAL_ENABLE_TESTS)

#Install 3rdparty binaries
if (WIN32)
    file(INSTALL
        "${Baikal_SOURCE_DIR}/3rdparty/glew/bin/x64/glew32.dll"
        "${Baikal_SOURCE_DIR}/3rdparty/glfw/bin/x64/glfw3.dll"
        "${Baikal_SOURCE_DIR}/3rdparty/oiio/bin/x64/OpenImageIO.dll"
        "${Baikal_SOURCE_DIR}/3rdparty/oiio/bin/x64/OpenImageIOD.dll"
        DESTINATION "${Baikal_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE}")
    #installation to Bin required when builded with nmake
    file(INSTALL
        "${Baikal_SOURCE_DIR}/3rdparty/glew/bin/x64/glew32.dll"
        "${Baikal_SOURCE_DIR}/3rdparty/glfw/bin/x64/glfw3.dll"
        "${Baikal_SOURCE_DIR}/3rdparty/oiio/bin/x64/OpenImageIO.dll"
        "${Baikal_SOURCE_DIR}/3rdparty/oiio/bin/x64/OpenImageIOD.dll"
        DESTINATION "${Baikal_BINARY_DIR}/Bin")
endif(WIN32)
