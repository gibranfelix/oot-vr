# SOH [Quest] Linux builds expect nlohmann_json, tinyxml2 and libzip to come from system
# packages. The Android path already FetchContent's them; do the same here when they are absent,
# so a machine that can cross-compile for the headset can also run the HOST build that generates
# soh.o2r without having to install distro packages first. Machines that do have them are
# unaffected - find_package wins.
include(FetchContent)

find_package(nlohmann_json QUIET)
if (NOT ${nlohmann_json_FOUND})
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(tinyxml2 QUIET)
if (NOT ${tinyxml2_FOUND})
    set(tinyxml2_BUILD_TESTING OFF)
    FetchContent_Declare(
        tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG 11.0.0
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(tinyxml2)
endif()

#=================== ImGui ===================
find_package(SDL2 REQUIRED)
target_link_libraries(ImGui PUBLIC SDL2::SDL2)

if (USE_OPENGLES)
    target_link_libraries(ImGui PUBLIC ${OPENGL_GLESv2_LIBRARY})
    add_compile_definitions(IMGUI_IMPL_OPENGL_ES3)
else()
    target_link_libraries(ImGui PUBLIC ${OPENGL_opengl_LIBRARY})
endif()
