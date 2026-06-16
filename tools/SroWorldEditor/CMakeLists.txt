cmake_minimum_required(VERSION 3.20)
project(SroWorldEditor LANGUAGES C CXX)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "SroWorldEditor must be built as Win32 (x86).")
endif()

if(MSVC)
    add_compile_options(/FS /MP)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../sro_ext_client/cmake")
find_package(DirectX REQUIRED)

set(IMGUI_ROOT "${CMAKE_CURRENT_LIST_DIR}/third-party/imgui-docking")
set(IMGUI_SOURCES
    ${IMGUI_ROOT}/imgui.cpp
    ${IMGUI_ROOT}/imgui_draw.cpp
    ${IMGUI_ROOT}/imgui_tables.cpp
    ${IMGUI_ROOT}/imgui_widgets.cpp
    ${IMGUI_ROOT}/backends/imgui_impl_dx9.cpp
    ${IMGUI_ROOT}/backends/imgui_impl_win32.cpp
)

set(SRO_PARSER_SOURCES
    src/sro/parsers/BmsParser.cpp
    src/sro/parsers/BmtParser.cpp
    src/sro/parsers/BsrParser.cpp
    src/sro/parsers/CpdParser.cpp
    src/sro/parsers/DdjParser.cpp
    src/sro/parsers/MapMeshParser.cpp
    src/sro/parsers/MapPlacementParser.cpp
    src/sro/parsers/NavMeshParser.cpp
    src/sro/parsers/MapProjectParser.cpp
)

set(SRO_INDEX_SOURCES
    src/sro/index/ObjectIndex.cpp
    src/sro/index/Tile2DIndex.cpp
)

set(SRO_RUNTIME_SOURCES
    src/sro/runtime/SroWorldSession.cpp
)

set(SRO_RENDER_SOURCES
    src/sro/render/SceneRenderer.cpp
)

set(SRO_MAP_SOURCES
    ../shared/sro_map/MapTextureCache.cpp
)

set(SRO_RENDERING_SOURCES
    src/sro/rendering/Texture.cpp
    src/sro/rendering/TerrainRenderer.cpp
    src/sro/rendering/MeshRenderer.cpp
    src/sro/rendering/NvmRenderer.cpp
    src/sro/rendering/WorldMapControl.cpp
    src/sro/rendering/MinimapWidget.cpp
)

set(APP_SOURCES
    src/main.cpp
    src/App.h
    src/EditorBuild.cpp
    src/EditorPublic.h
    src/EditorContext.cpp
    src/EditorContext.h
    src/EditorViewport.cpp
    src/EditorViewport.h
    src/ViewportFramebuffer.cpp
    src/ViewportFramebuffer.h
    src/EditorCamera.cpp
    src/EditorCamera.h
    src/RendererD3D9.cpp
    src/RendererD3D9.h
    src/ImGuiLayer.cpp
    src/ImGuiLayer.h
    src/core/Logger.cpp
    src/core/CommandHistory.cpp
    src/core/EditorAction.cpp
    src/core/EditorSession.cpp
    src/core/EntityPropertyBuilder.cpp
    src/core/EntityPropertyApplier.cpp
    src/world/World.cpp
    src/project/ProjectSerializer.cpp
    src/validation/Validator.cpp
    src/export/SqlExporter.cpp
    src/sro/cache/ClientDataCache.cpp
    src/sro/SroGlobals.cpp
    src/tools/SelectTool.cpp
    src/ui/MainMenuBar.cpp
    src/ui/Toolbar.cpp
    src/ui/ViewportPanel.cpp
    src/ui/WorldOutlinerBuild.cpp
    src/ui/PropertiesPanel.cpp
    src/ui/PropertySectionUi.cpp
    src/ui/AssetBrowserPanel.cpp
    src/ui/RegionManagerPanel.cpp
    src/ui/WorldMapPanel.cpp
    src/ui/ConsolePanel.cpp
    src/ui/ValidationPanel.cpp
    ${SRO_PARSER_SOURCES}
    ${SRO_INDEX_SOURCES}
    ${SRO_RUNTIME_SOURCES}
    ${SRO_RENDER_SOURCES}
    ${SRO_RENDERING_SOURCES}
    ${SRO_MAP_SOURCES}
    ${IMGUI_SOURCES}
)

add_executable(SroWorldEditor WIN32 ${APP_SOURCES})

target_compile_features(SroWorldEditor PRIVATE cxx_std_20)
target_compile_definitions(SroWorldEditor PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)

target_include_directories(SroWorldEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sro
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party
    ${IMGUI_ROOT}
    ${IMGUI_ROOT}/backends
    ${DirectX_INCLUDE_DIRS}
)

target_link_directories(SroWorldEditor PRIVATE ${DirectX_LIBRARY_PATHS})
target_link_libraries(SroWorldEditor PRIVATE d3d9 gdiplus shlwapi shell32 ole32 comdlg32)

set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../output")
set_target_properties(SroWorldEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}/Debug/${PROJECT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}/Release/${PROJECT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/Release/${PROJECT_NAME}"
)

add_executable(SroCoreTests tests/sro_core_tests.cpp
    src/sro/parsers/MapPlacementParser.cpp
    src/sro/parsers/MapProjectParser.cpp
)
target_compile_features(SroCoreTests PRIVATE cxx_std_20)
target_include_directories(SroCoreTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sro
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
)
target_compile_definitions(SroCoreTests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
set_target_properties(SroCoreTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/Release/${PROJECT_NAME}"
)
