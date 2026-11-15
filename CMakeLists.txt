
cmake_minimum_required(VERSION 3.25)

project(QtSeniorChallenge
    VERSION 1.0.0
    DESCRIPTION "DICOM app"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/installDCMTK.cmake)

find_package(Qt6 6.12.0 EXACT REQUIRED
    COMPONENTS Core Widgets
)

qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME}
    include/window.hpp
    include/dcm_file.hpp

    src/window.cpp
    src/dcm_file.cpp
    src/main.cpp
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(${PROJECT_NAME}
        PRIVATE /W4 /WX
    )
else()
    target_compile_options(${PROJECT_NAME}
        PRIVATE -Wall -Wextra -pedantic -Werror
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets
    
    dcmdata
    dcmimgle
    dcmimage
    dcmjpeg
    dcmjpls
)

# Install application
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Deploy Qt runtime dependencies
qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
)

install(SCRIPT ${deploy_script})
