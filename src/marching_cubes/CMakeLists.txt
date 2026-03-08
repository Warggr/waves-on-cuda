set(WHICH_MC33 "Own" CACHE STRING "Which MC33 library to use")
set_property(CACHE WHICH_MC33 PROPERTY STRINGS
	"Own"
	"DVega"
	"LCustodio"
)

if(NOT WHICH_MC33 STREQUAL "")
	message("Using MC33: ${WHICH_MC33}")
endif()

add_executable(_3dviewer viewer.cpp)
target_link_libraries(_3dviewer viewer)

# For things like -O3 and -g
get_target_property(C_FLAGS viewer COMPILE_OPTIONS)

if(WHICH_MC33 STREQUAL "DVega")
	include(ExternalProject)
	ExternalProject_Add(
		mc33_DVega
		URL http://jcgt.org/published/0008/03/01/marching_cubes_33.zip
		URL_HASH MD5=ccadad53e12b000c60c3c839cdeb14f7
		CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure"
		BUILD_IN_SOURCE   true
		BUILD_COMMAND     make CFLAGS=-D_ORTHO_GRD\ -DGRD_data_type=double\ ${C_FLAGS}\ -B
		INSTALL_COMMAND   ""
		TEST_COMMAND      ""
	)
	add_library(MC33.DVega wrappers/dvega.cpp)
	ExternalProject_Get_Property(mc33_DVega source_dir)
	target_link_libraries(MC33.DVega PRIVATE ${source_dir}/libMC33.a)
	target_include_directories(MC33.DVega PRIVATE ${source_dir})
	target_include_directories(MC33.DVega PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "..") # for grid.hpp
elseif(WHICH_MC33 STREQUAL "LCustodio")
	include(FetchContent)
	FetchContent_Declare(
		ModifiedMC33
		GIT_REPOSITORY https://github.com/liscustodio/modified_mc33
		GIT_TAG master
	)
	FetchContent_MakeAvailable(ModifiedMC33)
	enable_language(C)
	add_library(ModifiedMC33Lib
		${modifiedmc33_SOURCE_DIR}/source/MarchingCubes.cpp
		${modifiedmc33_SOURCE_DIR}/source/ply.c
	)
	target_include_directories(ModifiedMC33Lib PUBLIC ${modifiedmc33_SOURCE_DIR}/source)

	add_library(MC33.LCustodio wrappers/lcustodio.cpp)
	target_include_directories(MC33.LCustodio PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "..") # for grid.hpp
	target_link_libraries(MC33.LCustodio PRIVATE ModifiedMC33Lib)
elseif(WHICH_MC33 STREQUAL "Own")
    add_library(MC33.Own marching_cubes.cpp)
	target_link_libraries(MC33.Own viewer)
	target_link_libraries(MC33.Own marching_cubes_constants)
else()
	message(FATAL_ERROR "Error: invalid $$WHICH_MC33")
endif()

add_library(mc_renderer INTERFACE)
target_link_libraries(mc_renderer INTERFACE "MC33.${WHICH_MC33}")

target_link_libraries(_3dviewer "MC33.${WHICH_MC33}")

option(TIMING "Time marching cubes." off)
if(TIMING)
    target_compile_definitions("MC33.${WHICH_MC33}" PRIVATE TIMING)
endif()
