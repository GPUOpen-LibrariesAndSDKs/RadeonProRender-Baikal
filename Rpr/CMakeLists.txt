set(SOURCES
    Export.cpp
    Export.h
    RadeonProRender.cpp
    RadeonProRender.h
    RadeonProRender_CL.h
    RadeonProRender_GL.h
    Wrap.cpp)

set(WRAP_OBJECT_SOURCES
    WrapObject/CameraObject.cpp
    WrapObject/CameraObject.h
    WrapObject/ContextObject.cpp
    WrapObject/ContextObject.h
    WrapObject/Exception.h
    WrapObject/FramebufferObject.cpp
    WrapObject/FramebufferObject.h
    WrapObject/LightObject.cpp
    WrapObject/LightObject.h
    WrapObject/Materials/ArithmeticMaterialObject.cpp
    WrapObject/Materials/ArithmeticMaterialObject.h
    WrapObject/Materials/ImageMaterialObject.cpp
    WrapObject/Materials/ImageMaterialObject.h
    WrapObject/Materials/MaterialObject.cpp
    WrapObject/Materials/MaterialObject.h
    WrapObject/Materials/MultiBxdfMaterialObject.cpp
    WrapObject/Materials/MultiBxdfMaterialObject.h
    WrapObject/Materials/SingleBxdfMaterialObject.cpp
    WrapObject/Materials/SingleBxdfMaterialObject.h
    WrapObject/Materials/TextureMaterialObject.cpp
    WrapObject/Materials/TextureMaterialObject.h
    WrapObject/Materials/UberMaterialObject.cpp
    WrapObject/Materials/UberMaterialObject.h
    WrapObject/Materials/UnsupportedMaterialObject.cpp
    WrapObject/Materials/UnsupportedMaterialObject.h
    WrapObject/MatSysObject.cpp
    WrapObject/MatSysObject.h
    WrapObject/SceneObject.cpp
    WrapObject/SceneObject.h
    WrapObject/ShapeObject.cpp
    WrapObject/ShapeObject.h
    WrapObject/WrapObject.h)

set(UTILS_SOURCES
    Utils/config_manager.cpp
    Utils/config_manager.h
)

source_group("WrapObject" FILES ${WRAP_OBJECT_SOURCES})
source_group("Utils" FILES ${UTILS_SOURCES})

list(APPEND SOURCES ${WRAP_OBJECT_SOURCES} ${UTILS_SOURCES})

add_library(RadeonProRender SHARED ${SOURCES})
target_compile_definitions(RadeonProRender PRIVATE RPR_EXPORT_API)
target_compile_features(RadeonProRender PRIVATE cxx_std_14)
target_include_directories(RadeonProRender PUBLIC ${Baikal_SOURCE_DIR}/Rpr)
target_link_libraries(RadeonProRender PUBLIC Baikal BaikalIO OpenGL::GL GLEW::GLEW)

    
