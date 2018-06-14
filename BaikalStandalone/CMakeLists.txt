set(APPLICATION_SOURCES
    Application/application.cpp
    Application/application.h
    Application/app_utils.cpp
    Application/app_utils.h
    Application/cl_render.cpp
    Application/cl_render.h
    Application/gl_render.cpp
    Application/gl_render.h
    Application/uber_node.h
    Application/uber_node.cpp
    Application/uber_tree.h
    Application/uber_tree.cpp
    Application/graph_scheme.h
    Application/graph_scheme.cpp
    Application/material_explorer.h
    Application/material_explorer.cpp)

set(IMGUI_SORUCES
    ImGUI/imconfig.h
    ImGUI/imgui.cpp
    ImGUI/imgui.h
    ImGUI/imgui_demo.cpp
    ImGUI/imgui_draw.cpp
    ImGUI/imgui_impl_glfw_gl3.cpp
    ImGUI/imgui_impl_glfw_gl3.h
    ImGUI/imgui_internal.h
    ImGUI/stb_rect_pack.h
    ImGUI/stb_textedit.h
    ImGUI/stb_truetype.h)

set(MAIN_SOURCES main.cpp)
set(UTILS_SOURCES
    Utils/config_manager.cpp
    Utils/config_manager.h
    Utils/shader_manager.cpp
    Utils/shader_manager.h)

set(KERNEL_SOURCES
    Kernels/GLSL/simple.fsh
    Kernels/GLSL/simple.vsh)

set(SOURCES
    ${APPLICATION_SOURCES}
    ${IMGUI_SORUCES}
    ${MAIN_SOURCES}
    ${UTILS_SOURCES}
    ${KERNEL_SOURCES})

source_group("Application" FILES ${APPLICATION_SOURCES})
source_group("ImGUI" FILES ${IMGUI_SORUCES})
source_group("Utils" FILES ${UTILS_SOURCES})
source_group("Kernels\\GLSL" FILES ${KERNEL_SOURCES})
    
add_executable(BaikalStandalone ${SOURCES})
target_compile_features(BaikalStandalone PRIVATE cxx_std_14)
#Add project root since BaikalStandalone directly includes Baikal/* files
target_include_directories(BaikalStandalone
    PRIVATE ${Baikal_SOURCE_DIR}
    PRIVATE .)
target_link_libraries(BaikalStandalone PRIVATE Baikal BaikalIO glfw3::glfw3 OpenGL::GL GLEW::GLEW)

if (BAIKAL_ENABLE_DENOISER)
    target_compile_definitions(BaikalStandalone PUBLIC ENABLE_DENOISER)
endif(BAIKAL_ENABLE_DENOISER)

if (BAIKAL_ENABLE_GLTF)
    target_link_libraries(BaikalStandalone PRIVATE RadeonProRender RprSupport)
endif (BAIKAL_ENABLE_GLTF)

set_target_properties(BaikalStandalone
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${Baikal_SOURCE_DIR}/BaikalStandalone)

add_custom_target(BaikalStandaloneKernelsDir)

set(BAIKALSTANDALONE_KERNELS_DIR BaikalStandalone/Kernels)
FILE(TO_NATIVE_PATH ${Baikal_SOURCE_DIR}/${BAIKALSTANDALONE_KERNELS_DIR} BAIKALSTANDALONE_KERNELS_SRC)
FILE(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/${BAIKALSTANDALONE_KERNELS_DIR} BAIKALSTANDALONE_KERNELS_DST)

if (WIN32)
    add_custom_command(TARGET BaikalStandaloneKernelsDir POST_BUILD
        COMMAND IF NOT EXIST ${BAIKALSTANDALONE_KERNELS_DST} mklink /D ${BAIKALSTANDALONE_KERNELS_DST} ${BAIKALSTANDALONE_KERNELS_SRC}
        DEPENDS ${BAIKALSTANDALONE_KERNELS_DST}
    )
else ()
    add_custom_command(TARGET BaikalStandaloneKernelsDir POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${BAIKALSTANDALONE_KERNELS_SRC} ${BAIKALSTANDALONE_KERNELS_DST}
        DEPENDS ${BAIKALSTANDALONE_KERNELS_DST}
    )
endif ()

add_dependencies(BaikalStandalone ResourcesDir BaikalKernelsDir BaikalStandaloneKernelsDir)

if (WIN32)
    add_custom_command(TARGET BaikalStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BAIKALSTANDALONE_DLLS}
            "$<TARGET_FILE_DIR:BaikalStandalone>"
    )
endif ()

install(TARGETS BaikalStandalone RUNTIME DESTINATION bin)
if (WIN32)
    install(FILES ${BAIKALSTANDALONE_DLLS} DESTINATION bin)
endif ()
install(DIRECTORY ${RESOURCES_SRC} DESTINATION .)
install(DIRECTORY ${BAIKAL_KERNELS_SRC} DESTINATION Baikal)
install(DIRECTORY ${BAIKALSTANDALONE_KERNELS_SRC} DESTINATION BaikalStandalone)

#TODO: the following should be added to CMakeLists.txt of RadeonRays
#if (WIN32)
#    install(TARGETS RadeonRays RUNTIME DESTINATION bin)
#else ()
#    install(TARGETS RadeonRays LIBRARY DESTINATION lib)
#endif ()

#TODO: the following should be removed after the above being add to CMakeLists.txt of RadeonRays
if (WIN32)
    install(FILES $<TARGET_FILE_DIR:BaikalStandalone>/RadeonRays.dll DESTINATION bin)
else ()
    file(GLOB_RECURSE RADEONRAYS_SO ${CMAKE_BINARY_DIR}/libRadeonRays.*)
    install(FILES ${RADEONRAYS_SO} DESTINATION lib)
endif ()

