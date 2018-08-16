set(DATAGENERATOR_SOURCES
    Source/main.cpp
    Source/cmd_line_parser.h
    Source/cmd_line_parser.cpp
    Source/render.h
    Source/render.cpp
    Source/utils.h
    Source/config_loader.h
    Source/config_loader.cpp
    Source/input_info.h)

set(SOURCES
    ${DATAGENERATOR_SOURCES})

source_group("Source" FILES ${DATAGENERATOR_SOURCES})
    
add_executable(BaikalDataGenerator ${SOURCES})
target_compile_features(BaikalDataGenerator PRIVATE cxx_std_17)

#Add project root since BaikalStandalone directly includes Baikal/* files
target_include_directories(BaikalDataGenerator
    PRIVATE ${Baikal_SOURCE_DIR}
    PRIVATE .)
    
target_link_libraries(BaikalDataGenerator PRIVATE Baikal BaikalIO)

set_target_properties(BaikalDataGenerator
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${Baikal_SOURCE_DIR}/BaikalDataGenerator)

add_dependencies(BaikalDataGenerator ResourcesDir Copy3rdParty)

# Install target
install(TARGETS BaikalDataGenerator RUNTIME DESTINATION bin)
