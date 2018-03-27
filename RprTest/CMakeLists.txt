set(SOURCES
    main.cpp
    rprx_uberv2.h
    tiny_obj_loader.cpp
    tiny_obj_loader.h
    uberv2.h
    utils.h)

add_executable(RprTest ${SOURCES})
target_compile_features(RprTest PRIVATE cxx_std_14)
target_include_directories(RprTest PRIVATE .)
target_link_libraries(RprTest PRIVATE GTest RprSupport RadeonProRender)
set_target_properties(RprTest
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${Baikal_SOURCE_DIR}/RprTest)
