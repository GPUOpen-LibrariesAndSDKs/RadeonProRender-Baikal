set(SOURCES RprSupport.cpp)
    
add_library(RprSupport SHARED ${SOURCES})
target_compile_definitions(RprSupport PRIVATE RPR_EXPORT_API)
target_compile_features(RprSupport PRIVATE cxx_std_14)
target_include_directories(RprSupport PUBLIC ${RprSupport_HEADER})
target_link_libraries(RprSupport PUBLIC RadeonProRender)

    
