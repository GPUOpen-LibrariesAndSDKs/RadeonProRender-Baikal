set(SOURCES
    image_io.cpp
    image_io.h
    material_io.cpp
    material_io.h
    scene_binary_io.cpp
    scene_binary_io.h
    scene_io.cpp
    scene_io.h
    scene_test_io.cpp
    scene_obj_io.cpp
    )

if (BAIKAL_ENABLE_FBX)
    list(APPEND SOURCES scene_fbx_io.cpp)
endif (BAIKAL_ENABLE_FBX)

add_library(BaikalIO SHARED ${SOURCES})

if (BAIKAL_ENABLE_UBERV2)
    target_compile_definitions(BaikalIO PUBLIC ENABLE_UBERV2)
endif (BAIKAL_ENABLE_UBERV2)

target_compile_definitions(BaikalIO PRIVATE BAIKAL_EXPORT_API)
target_compile_features(BaikalIO PRIVATE cxx_std_14)
target_link_libraries(BaikalIO PUBLIC Baikal OpenImageIO::OpenImageIO)
target_include_directories(BaikalIO PUBLIC "${Baikal_SOURCE_DIR}/BaikalIO")
if (BAIKAL_ENABLE_FBX)
    target_link_libraries(BaikalIO PUBLIC fbxsdk::fbxsdk)
endif (BAIKAL_ENABLE_FBX)
