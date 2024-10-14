cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 20)
project(HGEditor)

set(CMAKE_REQUIRED_LIBRARIES "-L${atomic_lib_dir}" "-latomic")

find_package(PkgConfig REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
MESSAGE("${CMAKE_CURRENT_LIST_DIR}")
find_package(HaGame2 REQUIRED)

# set(CMAKE_TOOLCHAIN_FILE /home/henry/emsdk/)
if(EMSCRIPTEN)
    MESSAGE("BUILDING FOR THE WEB")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "public/")
else()
    find_package(X11 REQUIRED)
    find_package(ALSA REQUIRED)

    if(NOT X11_FOUND)
        message("ERROR: x11 not found")
    endif(NOT X11_FOUND)
endif()

find_package(OpenGL REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)
# pkg_search_module(GL REQUIRED gl)

set(IMGUI_PATH ${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui)
file(GLOB IMGUI_GLOB
        ${IMGUI_PATH}/imgui.h
        ${IMGUI_PATH}/imgui.cpp
        ${IMGUI_PATH}/imconfig.h
        ${IMGUI_PATH}/imgui_demo.cpp
        ${IMGUI_PATH}/imgui_draw.cpp
        ${IMGUI_PATH}/imgui_internal.h
        ${IMGUI_PATH}/imstb_rectpack.h
        ${IMGUI_PATH}/imstb_textedit.h
        ${IMGUI_PATH}/imstb_truetype.h
        ${IMGUI_PATH}/imgui_tables.cpp
        ${IMGUI_PATH}/imgui_widgets.cpp
        ${IMGUI_PATH}/misc/cpp/imgui_stdlib.h
        ${IMGUI_PATH}/misc/cpp/imgui_stdlib.cpp

        # specific bindings...
        ${IMGUI_PATH}/backends/imgui_impl_glfw.h
        ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
        ${IMGUI_PATH}/backends/imgui_impl_opengl3.h
        ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_PATH}/backends/imgui_impl_opengl3_loader.cpp)

add_library("imgui" STATIC ${IMGUI_GLOB})
target_include_directories("imgui" PUBLIC ${IMGUI_PATH})
target_link_libraries("imgui" PRIVATE glfw)

add_library(hge STATIC impl/events.cpp include/hge/components/componentExplorer.h impl/componentExplorer.cpp include/hge/components/entityViewer.h impl/entityViewer.cpp include/hge/components/entityTree.h impl/entityTree.cpp include/hge/components/assetBrowser.h impl/assetBrowser.cpp include/hge/hge.h include/hge/editor.h impl/editor.cpp include/hge/core/tool.h)
target_include_directories(hge PUBLIC include/)
target_include_directories(hge PUBLIC impl/)
target_include_directories(hge PUBLIC ${HAGAME2_INCLUDE_DIR})
target_include_directories(hge PUBLIC ${IMGUI_PATH})
target_include_directories(hge PUBLIC ${FIND_HAGAME2_PATHS}/thirdparty/glad/include/)

target_link_libraries(hge ${HAGAME2_LIBRARY})
target_link_libraries(hge imgui)
target_link_libraries(hge ${GLFW_LIBRARIES})

include_directories(${X11_INCLUDE_DIR})

add_executable(HGEditor main.cpp src/constants.h src/game.h src/game.cpp src/scenes/mainMenu.h src/scenes/mainMenu.cpp src/common/actionMap.h src/common/gameState.cpp src/common/persistentSettings.cpp src/common/ui.cpp src/common/enums/debugLevel.h include/hge/core/events.h impl/events.cpp src/scenes/runtime.h src/scenes/runtime.cpp)
target_include_directories(HGEditor PUBLIC ${HAGAME2_INCLUDE_DIR})
target_include_directories(HGEditor PUBLIC ${FIND_HAGAME2_PATHS}/thirdparty/asio/asio/include/)
target_include_directories(HGEditor PUBLIC ${FIND_HAGAME2_PATHS}/thirdparty/glad/include/)
target_include_directories(HGEditor PUBLIC ${FIND_HAGAME2_PATHS}/thirdparty/assimp/include/)
target_include_directories(HGEditor PUBLIC ${IMGUI_PATH})
target_link_libraries(HGEditor ${HAGAME2_LIBRARY})
target_link_libraries(HGEditor ${GLFW_LIBRARIES})
target_link_libraries(HGEditor ${OPENGL_LIBRARIES})
target_link_libraries(HGEditor imgui)
target_link_libraries(HGEditor ${X11_LIBRARIES})
target_link_libraries(HGEditor ${ALSA_LIBRARIES})
target_include_directories(HGEditor PUBLIC include/)
target_link_libraries(HGEditor hge)

# set(CMAKE_TOOLCHAIN_FILE /home/henry/emsdk/)
if(EMSCRIPTEN)
    set_target_properties(HGEditor PROPERTIES LINK_FLAGS "-DCMAKE_EXE_LINKER_FLAGS=-s -sMAX_WEBGL_VERSION=2 -sUSE_GLFW=3 -sTOTAL_MEMORY=512MB --preload-file ./../assets -sSTACK_SIZE=1000000")
endif()