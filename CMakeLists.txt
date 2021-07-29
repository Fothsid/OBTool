cmake_minimum_required(VERSION 3.20)
project(OBTool)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(FbxSdk)

add_subdirectory(libthanatos)
include_directories(libthanatos/include
					stb
					${FbxSdk_INCLUDE_DIRS}
					)

set(SRC
	src/main.cpp
	src/commands.c
	src/tim2.c
	src/tim2utils.c
	src/convert_to_fbx.cpp
	src/convert_to_nbd.cpp
	)

add_executable(OBTool ${SRC})
target_link_libraries(OBTool thanatos ${FbxSdk_LIBRARIES})