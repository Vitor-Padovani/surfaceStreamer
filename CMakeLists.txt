cmake_minimum_required(VERSION 3.12)
project(DeformablePointCloud)

set(CMAKE_CXX_STANDARD 11)

# Find VTK
find_package(VTK REQUIRED)
if(VTK_FOUND)
    include(${VTK_USE_FILE})
endif()

# Find OpenIGTLink
find_package(OpenIGTLink REQUIRED)
include(${OpenIGTLink_USE_FILE})

# Add executable
add_executable(DeformablePointCloud main.cpp)
target_link_libraries(DeformablePointCloud ${VTK_LIBRARIES} OpenIGTLink)

# For VTK 9+
if(VTK_VERSION VERSION_GREATER_EQUAL "9.0")
    vtk_module_autoinit(
        TARGETS DeformablePointCloud
        MODULES ${VTK_LIBRARIES}
    )
endif()