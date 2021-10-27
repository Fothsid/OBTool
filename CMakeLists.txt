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
	src/tristrip.cpp
	src/afs.c
	src/imageindexing.cpp

	src/convert_to_fbx.cpp
	src/convert_to_nbd.cpp
	src/extract_scenario_afs.cpp
	src/patch_nbd_textures.cpp

	# http://www.codercorner.com/Strips.htm
	src/striper/Adjacency.cpp
	src/striper/CustomArray.cpp
	src/striper/RevisitedRadix.cpp
	src/striper/StdAfx.cpp
	src/striper/Striper.cpp
	)

add_executable(OBTool ${SRC})
target_link_libraries(OBTool thanatos ${FbxSdk_LIBRARIES})
target_compile_options(OBTool PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/wd4312 /wd4311>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wno-sign-conversion -Wno-narrowing>
)