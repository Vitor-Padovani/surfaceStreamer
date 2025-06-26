cmake_minimum_required(VERSION 3.12)
project(DeformablePointCloud)

set(CMAKE_CXX_STANDARD 11)

# Encontrar VTK
find_package(VTK REQUIRED)
if(VTK_FOUND)
    include(${VTK_USE_FILE})
endif()

# Encontrar OpenIGTLink
find_package(OpenIGTLink REQUIRED)
include(${OpenIGTLink_USE_FILE})

# Adicionar executável
add_executable(DeformablePointCloud main.cpp)
target_link_libraries(DeformablePointCloud ${VTK_LIBRARIES} OpenIGTLink)

# Para VTK 9+
if(VTK_VERSION VERSION_GREATER_EQUAL "9.0")
    vtk_module_autoinit(
        TARGETS DeformablePointCloud
        MODULES ${VTK_LIBRARIES}
    )
endif()