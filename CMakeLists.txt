cmake_minimum_required(VERSION 3.10)
project(Echo)

# Set C++ version (Still using 23 for the features, just not the module syntax)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# 1. Tell CMake where your header files are
# 'include' is the folder name
include_directories(include)

# 2. Collect your source files
set(SOURCES 
    src/main.cpp
    src/main.cpp    
)

# 3. Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# 4. (Optional) If you need threading for your visualizer