cmake_minimum_required (VERSION 2.8.8)
project (Panther)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 11)


if (WIN32)
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )

    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MDd" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()

    add_definitions(-DWIN32 -D_CRT_SECURE_NO_WARNINGS -DNDEBUG -D_ITERATOR_DEBUG_LEVEL=0 -D_SCL_SECURE_NO_WARNINGS)
else(WIN32)
    set(CMAKE_CXX_FLAGS_DEFAULT "-O0 -g")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3")
endif(WIN32)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEFAULT "${CMAKE_CXX_FLAGS_DEFAULT}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")


if (NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "Choose the type of build by setting -DCMAKE_BUILD_TYPE=<type>, options are: Debug Release RelWithDebInfo MinSizeRel.")
endif()

# FIXME: check if CMAKE_BUILD_TYPE is set correctly as well

include_directories(files os rust/include settings syntax syntax/languages text testing third_party third_party/concurrent_queue third_party/json ui ui/controls ui/controls/basic ui/controls/main ui/controls/textfield utils)

if (WIN32)
	include_directories(os/windows)
endif(WIN32)

if (APPLE)
	include_directories(os/macos)
endif(APPLE)

# RE2 include directories
include_directories(third_party/re2)

# ICU include directories
include_directories(${ICU_BUILD_DIR}/include)

add_subdirectory(files)
add_subdirectory(os)
add_subdirectory(settings)
add_subdirectory(syntax)
add_subdirectory(testing)
add_subdirectory(text)
add_subdirectory(third_party)
add_subdirectory(ui)
add_subdirectory(utils)

add_executable(panther ${ALL_OBJECT_FILES})

# Enable ExternalProject CMake module
include(ExternalProject)

# Add rust_example as a CMake target
ExternalProject_Add(
    panther_rust
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build COMMAND cargo build --release
    BINARY_DIR "${CMAKE_SOURCE_DIR}/rust"
    INSTALL_COMMAND ""
    LOG_BUILD ON)

add_dependencies(panther panther_rust)

if (WIN32)
target_link_libraries(panther
    debug "${CMAKE_SOURCE_DIR}/rust/target/debug/pgrust.lib"
    optimized "${CMAKE_SOURCE_DIR}/rust/target/release/pgrust.lib"
    ws2_32 userenv advapi32)
    set_target_properties(panther PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
else(WIN32)
target_link_libraries(panther
    debug "${CMAKE_SOURCE_DIR}/rust/target/debug/libpgrust.a"
    optimized "${CMAKE_SOURCE_DIR}/rust/target/release/libpgrust.a"
    )
endif(WIN32)

#icu libraries
if (WIN32)
    target_link_libraries(panther ${ICU_BUILD_DIR}/lib/sicudt.lib ${ICU_BUILD_DIR}/lib/sicuin.lib ${ICU_BUILD_DIR}/lib/sicuio.lib ${ICU_BUILD_DIR}/lib/sicutu.lib ${ICU_BUILD_DIR}/lib/sicuuc.lib)
else(WIN32)
    target_link_libraries(panther ${ICU_BUILD_DIR}/lib/libicudata.a   ${ICU_BUILD_DIR}/lib/libicuio.a ${ICU_BUILD_DIR}/lib/libicutu.a ${ICU_BUILD_DIR}/lib/libicui18n.a ${ICU_BUILD_DIR}/lib/libicuuc.a)
endif(WIN32)



#skia
if (WIN32)
    target_link_libraries(panther ${SKIA_BUILD_DIR}/skia.lib)
else(WIN32)
    target_link_libraries(panther ${SKIA_BUILD_DIR}/libskia.a)
endif(WIN32)

if (APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AppKit -lobjc")
endif(APPLE)
