# Library

set(DATAGENERATOR_LIB_SOURCES
    Source/data_generator.h
    Source/data_generator.cpp
    Source/devices.h
    Source/devices.cpp
    Source/filesystem.h
    Source/render.h
    Source/render.cpp
    Source/utils.h
)

add_library(BaikalDataGeneratorLib SHARED ${DATAGENERATOR_LIB_SOURCES})

target_compile_features(BaikalDataGeneratorLib PRIVATE cxx_std_17)

target_compile_definitions(BaikalDataGeneratorLib PRIVATE RPR_EXPORT_API)

target_include_directories(BaikalDataGeneratorLib
    PRIVATE .
    PRIVATE ${Baikal_SOURCE_DIR}
    PRIVATE ${Baikal_SOURCE_DIR}/Rpr
    PRIVATE ${Baikal_SOURCE_DIR}/RadeonRays/include
)

target_link_libraries(BaikalDataGeneratorLib PRIVATE RadeonProRender64 Baikal BaikalIO)

set_target_properties(BaikalDataGeneratorLib
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${Baikal_SOURCE_DIR}/BaikalDataGeneratorLib)

add_dependencies(BaikalDataGeneratorLib ResourcesDir Copy3rdParty)

if (WIN32)
    install(TARGETS BaikalDataGeneratorLib RUNTIME DESTINATION bin)
else ()
    install(TARGETS BaikalDataGeneratorLib LIBRARY DESTINATION lib)
endif ()


# Application

set(DATAGENERATOR_APP_SOURCES
    Source/cmd_line_parser.h
    Source/cmd_line_parser.cpp
    Source/devices.h
    Source/devices.cpp
    Source/object_loader.h
    Source/object_loader.cpp
    Source/logging.h
    Source/logging.cpp
    Source/main.cpp
)

add_executable(BaikalDataGenerator ${DATAGENERATOR_APP_SOURCES})

target_compile_features(BaikalDataGenerator PRIVATE cxx_std_17)

target_include_directories(BaikalDataGenerator
    PRIVATE ${Baikal_SOURCE_DIR}
    PRIVATE .
)

target_link_libraries(BaikalDataGenerator PRIVATE BaikalDataGeneratorLib RadeonProRender64 Baikal)

set_target_properties(BaikalDataGenerator
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${Baikal_SOURCE_DIR}/BaikalDataGenerator)

install(TARGETS BaikalDataGenerator RUNTIME DESTINATION bin)
