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

set(MAIN_SOURCES Source/main.cpp)

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

if (WIN32)
    add_custom_command(TARGET BaikalDataGenerator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${IO_DLLS}
            "$<TARGET_FILE_DIR:BaikalDataGenerator>"
    )
endif ()

install(TARGETS BaikalDataGenerator RUNTIME DESTINATION bin)

if (WIN32)
    install(FILES ${IO_DLLS} DESTINATION bin)
endif ()
install(DIRECTORY ${RESOURCES_SRC} DESTINATION .)

if (WIN32)
    install(FILES $<TARGET_FILE_DIR:BaikalDataGenerator>/RadeonRays.dll DESTINATION bin)
else ()
    file(GLOB_RECURSE RADEONRAYS_SO ${CMAKE_BINARY_DIR}/libRadeonRays.*)
    install(FILES ${RADEONRAYS_SO} DESTINATION lib)
endif ()