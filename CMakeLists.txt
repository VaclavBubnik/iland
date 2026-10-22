# Copyright 2020 VUKOZ
#
# This file is part of 3D Forest.
#
# 3D Forest is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# 3D Forest is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with 3D Forest.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.24)

project(iland LANGUAGES C CXX)

option(ILAND_STATIC_RUNTIME "Link the Windows runtime statically" ON)

find_package(Threads REQUIRED)

# Keep the executable beside the shared library so it can load the library at runtime.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory(3rdparty/sqlite)
add_subdirectory(3rdparty/pugixml)
add_subdirectory(3rdparty/quickjs)

add_subdirectory(src)
add_subdirectory(apps)
