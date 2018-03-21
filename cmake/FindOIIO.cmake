set(OIIO_DIR ${Baikal_SOURCE_DIR}/3rdparty/oiio)

find_library(OIIO_LIB OpenImageIO HINTS ${OIIO_DIR}/lib/x64)

add_library(OpenImageIO SHARED IMPORTED)
set_target_properties(OpenImageIO PROPERTIES LOCATION ${OIIO_LIB})
set_target_properties(OpenImageIO PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${OIIO_DIR}/include
    INTERFACE_LINK_LIBRARIES OpenImageIO::OpenImageIO)
    
