set(OIIO_DIR ${Baikal_SOURCE_DIR}/3rdparty/oiio)


find_library(OIIO_LIB OpenImageIO PATHS ${OIIO_DIR}/lib/x64)
find_path(OIIO_INCLUDE_DIR NAMES OpenImageIO/oiioversion.h PATHS "${OIIO_DIR}/include")

find_package_handle_standard_args(OIIO DEFAULT_MSG OIIO_LIB OIIO_INCLUDE_DIR)

add_library(OpenImageIO SHARED IMPORTED)
set_target_properties(OpenImageIO PROPERTIES IMPORTED_LOCATION ${OIIO_LIB})
set_target_properties(OpenImageIO PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${OIIO_INCLUDE_DIR})

