cmake_minimum_required(VERSION 3.12)

set(SFML_STATIC_LIBRARIES TRUE)
find_package(SFML 2.5 REQUIRED COMPONENTS graphics)

set(SOURCE_FILES
		main.cpp
		sound.cpp
	)

set(HEADER_FILES
		sound.h
	)

add_executable(sfml-synth ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(sfml-synth sfml-graphics synth imgui imgui-sfml Winmm.lib)
target_include_directories(sfml-synth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(sfml-synth PUBLIC cxx_std_17)

source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Header Files" FILES ${HEADER_FILES})

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(sfml-synth PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(sfml-synth PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
