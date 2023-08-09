cmake_minimum_required(VERSION 3.12)

project(files_to_burn VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define your source files
set(SOURCES
    main.cpp
)

add_executable(files_to_burn ${SOURCES})

# Add include directories
target_include_directories(files_to_burn PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Add -lpthread
target_link_libraries(files_to_burn PRIVATE pthread)

# Add installation
install(TARGETS files_to_burn DESTINATION bin)

# define DEBUG macro
# target_compile_definitions(files_to_burn PRIVATE DEBUG)
