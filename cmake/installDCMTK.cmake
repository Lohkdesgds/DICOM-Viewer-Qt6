
include(FetchContent)

# ---------------------------------------------------------------------------
# DCMTK configuration
# ---------------------------------------------------------------------------

set(BUILD_APPS OFF CACHE BOOL "Build DCMTK applications" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build DCMTK static libraries" FORCE)

set(DCMTK_WITH_TIFF OFF CACHE BOOL "" FORCE)
set(DCMTK_WITH_PNG OFF CACHE BOOL "" FORCE)
set(DCMTK_WITH_XML OFF CACHE BOOL "" FORCE)
set(DCMTK_WITH_ZLIB ON CACHE BOOL "" FORCE)
set(DCMTK_WITH_OPENSSL OFF CACHE BOOL "" FORCE)
set(DCMTK_WITH_SNDFILE OFF CACHE BOOL "" FORCE)
set(DCMTK_WITH_WRAP OFF CACHE BOOL "" FORCE)
set(DCMTK_WITH_OPENJPEG OFF CACHE BOOL "" FORCE)

set(DCMTK_WITH_THREADS ON CACHE BOOL "" FORCE)
set(DCMTK_ENABLE_STL ON CACHE BOOL "" FORCE)

set(DCMTK_WITH_DOXYGEN OFF CACHE BOOL "" FORCE)
set(DCMTK_GENERATE_DOXYGEN_TAGFILE OFF CACHE BOOL "" FORCE)

# Avoid requiring an external DICOM dictionary file.
set(DCMTK_DEFAULT_DICT builtin CACHE STRING "" FORCE)

# Embed character conversion data into the library.
set(DCMTK_ENABLE_BUILTIN_OFICONV_DATA ON CACHE BOOL "" FORCE)

# Build only the modules required by the application.
set(
    DCMTK_MODULES
    "ofstd;oflog;oficonv;dcmdata;dcmimgle;dcmimage;dcmjpeg;dcmjpls"
    CACHE STRING "DCMTK modules to build"
    FORCE
)

# ---------------------------------------------------------------------------
# Fetch DCMTK
# ---------------------------------------------------------------------------

FetchContent_Declare(
    dcmtk
    GIT_REPOSITORY https://github.com/DCMTK/dcmtk.git
    GIT_TAG DCMTK-3.7.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(dcmtk)
