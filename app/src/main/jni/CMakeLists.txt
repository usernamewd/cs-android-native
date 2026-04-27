cmake_minimum_required(VERSION 3.22.1)
project(csnative LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------
# Hardening / size flags. These are deliberately strong:
#   * hidden visibility        -> minimal exported symbol table
#   * function/data sections   -> dead-code stripping per symbol
#   * --gc-sections            -> linker drops unused
#   * full RELRO + BIND_NOW    -> relocation hardening
#   * stack protector strong   -> stack canaries
#   * _FORTIFY_SOURCE=2        -> libc fortification
#   * -fno-rtti -fno-exceptions-> no class metadata in binary
#   * -ffile-prefix-map        -> scrub absolute paths
# -------------------------------------------------------------
set(HARDENING_C_FLAGS
    "-fvisibility=hidden"
    "-fvisibility-inlines-hidden"
    "-ffunction-sections"
    "-fdata-sections"
    "-fstack-protector-strong"
    "-fPIC"
    "-D_FORTIFY_SOURCE=2"
    "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=."
    "-Wno-unused-parameter"
    "-Wno-unused-function"
)

set(HARDENING_CXX_FLAGS
    ${HARDENING_C_FLAGS}
    "-fno-rtti"
    "-fno-exceptions"
)

set(HARDENING_LINK_FLAGS
    "-Wl,--gc-sections"
    "-Wl,-z,relro"
    "-Wl,-z,now"
    "-Wl,-z,noexecstack"
    "-Wl,--exclude-libs,ALL"
    "-Wl,--no-undefined"
    "-Wl,--build-id=none"
    "-Wl,--strip-all"
    "-Wl,-s"
    "-Wl,--hash-style=gnu"
)

# Optional OLLVM passes. If your toolchain is a custom OLLVM/Hikari build,
# pass -DUSE_OLLVM=ON and these flags get appended.
option(USE_OLLVM "Enable Obfuscator-LLVM passes (requires custom toolchain)" OFF)
if(USE_OLLVM)
    list(APPEND HARDENING_CXX_FLAGS
        "-mllvm" "-fla"           # control-flow flattening
        "-mllvm" "-bcf"           # bogus control flow
        "-mllvm" "-bcf_loop=2"
        "-mllvm" "-bcf_prob=40"
        "-mllvm" "-sub"           # instruction substitution
        "-mllvm" "-sub_loop=2"
        "-mllvm" "-sobf"          # string obfuscation
        "-mllvm" "-split"         # basic-block splitting
        "-mllvm" "-split_num=3"
    )
endif()

# -------------------------------------------------------------
# Sources.
# -------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SRC
    engine/*.cpp engine/*.c
    game/*.cpp
    hardening/*.cpp
    util/*.cpp
)

add_library(csnative SHARED
    jni_bridge.cpp
    ${ENGINE_SRC}
)

target_include_directories(csnative PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(csnative PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${HARDENING_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${HARDENING_CXX_FLAGS}>
)

target_link_options(csnative PRIVATE ${HARDENING_LINK_FLAGS})

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles3-lib GLESv3)

target_link_libraries(csnative
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles3-lib}
)

# Strip again post-link as belt-and-suspenders. CMAKE_STRIP is set by the NDK.
add_custom_command(TARGET csnative POST_BUILD
    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:csnative> || true
    COMMENT "Final strip pass on libcsnative.so"
)
