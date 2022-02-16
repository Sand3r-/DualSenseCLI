cmake_minimum_required (VERSION 3.21)

# Define an executable target
set(PROJECT_NAME DualSenseCLI)
set(TARGET_NAME ${PROJECT_NAME})
project(${PROJECT_NAME})

# Add cmake/ includes
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_executable(${TARGET_NAME})

# Specify the required C standard
set_target_properties(${TARGET_NAME} PROPERTIES C_STANDARD 17)
set_target_properties(${TARGET_NAME} PROPERTIES CMAKE_C_STANDARD_REQUIRED True)

# Find Windows HID API
include(FindWinHID)

if(${WINHID_FOUND})
    message(STATUS "Windows HID was found.")
else()
    message(FATAL_ERROR "Windows HID wasn't found. Aborting...")
endif()

# Link HID to the project
target_include_directories(${TARGET_NAME} PRIVATE ${WINHID_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PRIVATE ${WINHID_LIBRARIES})

# Add all of the source code for the exe itself
add_subdirectory(src)