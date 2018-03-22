cmake_minimum_required(VERSION 3.8)

project(Baikal CXX)

option(BAIKAL_ENABLE_RAYMASK "Enable visibility flags for shapes (slows down an intersector)" OFF)
option(BAIKAL_ENABLE_DENOISER "Use denoising on output" OFF)
option(BAIKAL_ENABLE_FBX "Enable FBX import" OFF)
option(BAIKAL_ENABLE_GLTF "Enable glTF import" OFF)
option(BAIKAL_ENABLE_RPR "Enable RadeonProRender API lib" OFF)
#option(BAIKAL_EMBED_KERNELS "Embed CL kernels into binary module" OFF)
option(BAIKAL_ENABLE_UBERV2 "Enable UberV2 support" OFF)
option(BAIKAL_ENABLE_TESTS "Enable Baikal tests" ON)
option(BAIKAL_ENABLE_STANDALONE "Enable standalone application build" ON)
option(BAIKAL_ENABLE_RPR_TESTS "Enable RPR API tests build (requires BAIKAL_ENABLE_RPR to be turned on)" ON)



#global settings
if (WIN32)
    add_definitions(/MP -D_CRT_SECURE_NO_WARNINGS)
elseif (UNIX)
    add_definitions(-fvisibility=hidden)
endif (WIN32)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#Find required packages
find_package(Threads REQUIRED)
find_package(OIIO REQUIRED)

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

add_subdirectory(RadeonRays)

add_subdirectory(Baikal)

if (BAIKAL_ENABLE_STANDALONE)
    find_package(GLFW3 REQUIRED)
    find_package(OpenGL REQUIRED)

    find_package(GLEW REQUIRED)
    add_subdirectory(BaikalStandalone)
endif (BAIKAL_ENABLE_STANDALONE)

