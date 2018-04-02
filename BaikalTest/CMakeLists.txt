set(SOURCES
    aov.h
    basic.h
    camera.h
    input_maps.h
    internal.h
    light.h
    main.cpp
    material.h
    test_scenes.h
    uberv2.h)

add_executable(BaikalTest ${SOURCES})
target_compile_features(BaikalTest PRIVATE cxx_std_14)
target_include_directories(BaikalTest PRIVATE .)
target_link_libraries(BaikalTest PRIVATE Baikal BaikalIO GTest)
set_target_properties(BaikalTest
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${Baikal_SOURCE_DIR}/BaikalTest)
