set(FbxSdk_LibVersions)
foreach(year RANGE 2000 2100)
	foreach (ver RANGE 0 10)
		list(APPEND FbxSdk_LibVersions ${year}.${ver})
	endforeach()
endforeach()

find_path(FbxSdk_ROOT
		  NAMES "include/fbxsdk.h"
		  HINTS $ENV{ProgramFiles}
		  		$ENV{ProgramFiles}/Autodesk
		  		$ENV{ProgramFiles}/Autodesk/FBX
		  		"$ENV{ProgramFiles}/Autodesk/FBX/FBX SDK"
		  PATH_SUFFIXES fbx-sdk
		  				Autodesk
		  				FBX
						"FBX SDK"
						${FbxSdk_LibVersions})

message(STATUS "FBX SDK => ${FbxSdk_ROOT}")
if (${FbxSdk_ROOT} MATCHES "FbxSdk_ROOT-NOTFOUND")
	message(FATAL_ERROR "Could not find the FBX SDK directory")
endif()

set(FbxSdk_INCLUDE_DIRS "${FbxSdk_ROOT}/include")
message(STATUS "FBX SDK include dirs => ${FbxSdk_INCLUDE_DIRS}")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BUILD_ARCH "x64")
else()
	set(BUILD_ARCH "x86")
endif()

if (MSVC)
	find_library(FbxSdk_LIBRARY_RELEASE "libfbxsdk.lib" HINTS "${FbxSdk_ROOT}/lib/vs2019/${BUILD_ARCH}/release")
	find_library(FbxSdk_LIBRARY_DEBUG "libfbxsdk.lib" HINTS "${FbxSdk_ROOT}/lib/vs2019/${BUILD_ARCH}/debug")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_library(FbxSdk_LIBRARY_RELEASE "libfbxsdk.a" HINTS "${FbxSdk_ROOT}/lib/gcc/${BUILD_ARCH}/release")
	find_library(FbxSdk_LIBRARY_DEBUG "libfbxsdk.a" HINTS "${FbxSdk_ROOT}/lib/gcc/${BUILD_ARCH}/debug")
else() # Mac
	find_library(FbxSdk_LIBRARY_RELEASE "libfbxsdk.a" HINTS "${FbxSdk_ROOT}/lib/clang/ub/release")
	find_library(FbxSdk_LIBRARY_DEBUG "libfbxsdk.a" HINTS "${FbxSdk_ROOT}/lib/clang/ub/debug")
endif()

if (${CMAKE_BUILD_TYPE} MATCHES "Release")
	set(FbxSdk_LIBRARIES ${FbxSdk_LIBRARY_RELEASE})
else()
	set(FbxSdk_LIBRARIES ${FbxSdk_LIBRARY_DEBUG})
endif()

message(STATUS "FBX library => ${FbxSdk_LIBRARIES}")
