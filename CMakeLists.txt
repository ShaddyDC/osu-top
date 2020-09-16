cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(osu_top LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/modules")

find_package(Magnum REQUIRED GL)
find_package(Corrade REQUIRED Utility)

if(CORRADE_TARGET_EMSCRIPTEN)
    find_package(Magnum REQUIRED Sdl2Application)   # Todo: Move this to emscripten
    # find_package(Magnum REQUIRED EmscriptenApplication)
else()
    find_package(Threads REQUIRED)
    find_package(Magnum REQUIRED Sdl2Application)

    # WebGL doesn't need OpenSSL because of javascript
    find_package(OpenSSL REQUIRED)
endif()

add_subdirectory(libs)

add_executable(osu_top
    src/main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
    src/config_manager.cpp src/config_manager.h src/request_maps.cpp src/request_maps.h src/web_request.cpp src/web_request.h src/score.h)

# allow inclusion of headers from other folders (version.cpp)
target_include_directories(osu_top PRIVATE src)

set_target_properties(osu_top PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CORRADE_USE_PEDANTIC_FLAGS ON
)

if(CORRADE_TARGET_EMSCRIPTEN)
    set_target_properties(osu_top PROPERTIES
        LINK_FLAGS "-lidbfs.js -s EXPORTED_FUNCTIONS='[_main, _loaded_callback, _async_wget_delete]' -s EXPORTED_RUNTIME_METHODS='[ccall]' -s ASYNCIFY=1 -s ASYNCIFY_STACK_SIZE=65535"
    )
    # -lidbfs.js for dbfs file system
    # EXPORTED_FUNCTIONS to allow access from js
    # EXPORTED_RUNTIME_METHODS to allow access from js
    # ASYNCIFY to allow async js calls
    # ASYNCIFY_STACK_SIZE to avoid "RuntimeError: unreachable executed" in async code
endif()

target_link_libraries(osu_top PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    MagnumIntegration::ImGui
    ImGui::imgui_stdlib
    Corrade::Utility
    nlohmann_json::nlohmann_json
)

# WebGL doesn't need OpenSSL because of javascript
if(NOT CORRADE_TARGET_EMSCRIPTEN)
    target_link_libraries(osu_top PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        httplib::httplib
        Threads::Threads
    )
endif()

# Include Build Information

# get git commit hash
execute_process(COMMAND
        "${GIT_EXECUTABLE}" describe --always --abbrev=40 --dirty
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_SHA1
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        )

# get git commit time
execute_process(COMMAND
        "${GIT_EXECUTABLE}" log -1 --format=%ad --date=iso
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_DATE
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
        )

# get build time
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")

# apply variables
configure_file(src/version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/version.cpp @ONLY)


# For Emscripten copy the boilerplate next to the executable so it can be run
# directly from the build dir; provide an install target as well
if(CORRADE_TARGET_EMSCRIPTEN)
    add_custom_command(TARGET osu_top POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/index.html
        $<TARGET_FILE_DIR:osu_top>
    )

    install(TARGETS osu_top DESTINATION ${MAGNUM_DEPLOY_PREFIX})
    install(FILES
            index.html
            ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
            ${MAGNUM_WEBAPPLICATION_CSS}
            DESTINATION ${MAGNUM_DEPLOY_PREFIX})
    install(FILES
            $<TARGET_FILE_DIR:osu_top>/osu_top.js.mem
            $<TARGET_FILE_DIR:osu_top>/osu_top.wasm
            DESTINATION ${MAGNUM_DEPLOY_PREFIX} OPTIONAL)
endif()

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT osu_top)
