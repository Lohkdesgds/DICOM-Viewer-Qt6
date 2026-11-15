cmake_minimum_required(VERSION 3.16)

#Project name
set(TARGET_NAME Temp)

project(${TARGET_NAME} VERSION 1.0.0 DESCRIPTION "DICOM app")

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(${TARGET_NAME}
    include/window.hpp
    src/window.cpp
    src/main.cpp
)
    
if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_include_directories(${TARGET_NAME} PRIVATE ${Qt6_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Widgets)