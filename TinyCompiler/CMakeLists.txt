cmake_minimum_required(VERSION 3.16)
project(TinyCompiler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# 源文件列表
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/scanner.cpp
    src/parser.cpp
    src/treenode.cpp
    src/syntaxtreemodel.cpp
    src/syntaxtreeview.cpp
    src/highlighter.cpp
)

set(HEADERS
    src/mainwindow.h
    src/globals.h
    src/scanner.h
    src/parser.h
    src/treenode.h
    src/syntaxtreemodel.h
    src/syntaxtreeview.h
    src/highlighter.h
)

# 创建可执行文件
add_executable(TinyCompiler ${SOURCES} ${HEADERS})

# 链接Qt库
target_link_libraries(TinyCompiler PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui)

# Windows下设置为窗口应用程序（不显示控制台）
if(WIN32)
    set_target_properties(TinyCompiler PROPERTIES WIN32_EXECUTABLE TRUE)
endif()