cmake_minimum_required(VERSION 3.16)

project(BankSystem)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(PROJECT_SOURCES
    src/main.cpp
    src/main.h
    src/account_system.cpp
    src/account_system.h
)

set(SOURCE_LIB_SQL
    sqlite3/sqlite3.h
    sqlite3/sqlite3.c
)

set(SOURCE_LIB_IMGUI
    imgui/imconfig.h
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_internal.h
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
    imgui/backends/imgui_impl_dx11.cpp
    imgui/backends/imgui_impl_dx11.h
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_win32.h
)

add_library(sqlite STATIC ${SOURCE_LIB_SQL})
add_library(imgui STATIC ${SOURCE_LIB_IMGUI})

target_include_directories(sqlite PUBLIC sqlite3)
target_include_directories(imgui PUBLIC imgui imgui/backends)


add_executable(BankSystem  ${PROJECT_SOURCES})

target_include_directories(BankSystem PRIVATE .)

target_link_libraries(BankSystem PRIVATE 
    sqlite 
    imgui
    d3d11          
    d3dcompiler    
    dwmapi         
)