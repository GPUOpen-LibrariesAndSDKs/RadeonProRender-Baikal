set(APPLICATION_SOURCES
    Application/application.cpp
    Application/application.h
    Application/app_utils.cpp
    Application/app_utils.h
    Application/cl_render.cpp
    Application/cl_render.h
    Application/gl_render.cpp
    Application/gl_render.h)

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

set(SOURCES
    ${APPLICATION_SOURCES}
    ${IMGUI_SORUCES}
    ${MAIN_SOURCES}
    ${UTILS_SOURCES})

source_group("application" FILES ${APPLICATION_SOURCES})
source_group("ImGUI" FILES ${IMGUI_SORUCES})
source_group("utils" FILES ${UTILS_SOURCES})
    
add_executable(BaikalStandalone ${SOURCES})
target_compile_features(BaikalStandalone PRIVATE cxx_std_14)
#Add project root since BaikalStandalone directly includes Baikal/* files
target_include_directories(BaikalStandalone
    PRIVATE ${Baikal_SOURCE_DIR}
    PRIVATE .)
target_link_libraries(BaikalStandalone PRIVATE Baikal glfw3::glfw3 OpenGL::GL GLEW::GLEW)

if (BAIKAL_ENABLE_GLTF)
    target_link_libraries(BaikalStandalone PRIVATE RadeonProRender RprSupport)
endif (BAIKAL_ENABLE_GLTF)

    
