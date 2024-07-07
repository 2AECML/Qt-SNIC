# CMakeList.txt: use_gdal 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 启用 Qt 工具链的自动处理功能
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("use_gdal")

# 包含 vcpkg 脚本
include("D:/VCPKG/vcpkg/scripts/buildsystems/vcpkg.cmake")

# 设置目录
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# 添加头文件目录
include_directories(${INCLUDE_DIR})

# 将源代码添加到此项目的可执行文件。
add_executable (use_gdal 
	${INCLUDE_DIR}/main.h
	${INCLUDE_DIR}/snic.h
	${INCLUDE_DIR}/CustomGraphicsView.h
	${SOURCE_DIR}/main.cpp
	${SOURCE_DIR}/snic.c
	${SOURCE_DIR}/SegmentationResult.cpp
	${SOURCE_DIR}/CustomGraphicsView.cpp
	${SOURCE_DIR}/PolygonManager.cpp
	${SOURCE_DIR}/CustomPolygonItem.cpp
	${SOURCE_DIR}/WorkerThread.cpp)

# 寻找 Qt 库
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# 检查 Qt 包含目录和库
message(STATUS "Qt5Core include dirs: ${Qt5Core_INCLUDE_DIRS}")
message(STATUS "Qt5Core libraries: ${Qt5Core_LIBRARIES}")
message(STATUS "Qt5Widgets include dirs: ${Qt5Widgets_INCLUDE_DIRS}")
message(STATUS "Qt5Widgets libraries: ${Qt5Widgets_LIBRARIES}")

# 寻找 GDAL 库
find_package(GDAL REQUIRED)

# 检查 GDAL 包含目录和库
message(STATUS "GDAL include dirs: ${GDAL_INCLUDE_DIRS}")
message(STATUS "GDAL libraries: ${GDAL_LIBRARIES}")

# 寻找 OpenCV 库
find_package(OpenCV REQUIRED)

# 检查 OpenCV 包含目录和库
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBRARIES}")

# 添加库目录
include_directories(${Qt5Core_INCLUDE_DIRS} 
	${Qt5Widgets_INCLUDE_DIRS} 
	${OpenCV_INCLUDE_DIRS} 
	${GDAL_INCLUDE_DIR})

# 链接库
target_link_libraries(use_gdal ${Qt5Core_LIBRARIES} 
	${Qt5Widgets_LIBRARIES} 
	${OpenCV_LIBRARIES} 
	${GDAL_LIBRARIES})


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET use_gdal PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
