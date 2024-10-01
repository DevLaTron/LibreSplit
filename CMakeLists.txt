# Set the minimum required CMake version and the project name
cmake_minimum_required(VERSION 3.10)
project(libresplit C)

# Set the C standard and other compiler flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -pthread -Wall -Wno-unused-parameter")

# Find the required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(JANSSON REQUIRED jansson)
pkg_check_modules(LUAJIT REQUIRED luajit)

# Add include directories from the found packages
include_directories(${GTK3_INCLUDE_DIRS} ${X11_INCLUDE_DIRS} ${JANSSON_INCLUDE_DIRS} ${LUAJIT_INCLUDE_DIRS})

# Add compile options from the found packages
add_compile_options(${GTK3_CFLAGS_OTHER} ${X11_CFLAGS_OTHER} ${JANSSON_CFLAGS_OTHER} ${LUAJIT_CFLAGS_OTHER})

# Set the source directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(COMPONENT_DIR ${SRC_DIR}/component)

# Obtain the list of source files
file(GLOB SOURCES ${SRC_DIR}/*.c)
file(GLOB COMPONENTS ${COMPONENT_DIR}/*.c)

# Set the output directory for object files
set(OBJ_DIR ${CMAKE_BINARY_DIR}/obj)

# Custom command to generate main.h from main.css using xxd
add_custom_command(
    OUTPUT ${SRC_DIR}/main.h
    COMMAND xxd --include main.css > main.h
    WORKING_DIRECTORY ${SRC_DIR}
    DEPENDS ${SRC_DIR}/main.css
    COMMENT "Generating main.h from main.css"
    )
# Add a custom target for main.h
add_custom_target(generate_main_h DEPENDS ${SRC_DIR}/main.h)

# Add executable target and link the necessary libraries
add_executable(libresplit ${SOURCES} ${COMPONENTS} ${SRC_DIR}/main.h)
add_dependencies(libresplit generate_main_h)

target_link_libraries(libresplit ${GTK3_LIBRARIES} ${X11_LIBRARIES} ${JANSSON_LIBRARIES} ${LUAJIT_LIBRARIES})

# Installation rules
install(TARGETS libresplit DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES libresplit.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)

# Install the icon for different sizes
foreach(size 16 22 24 32 36 48 64 72 96 128 256 512)
    install(CODE "execute_process(COMMAND rsvg-convert -w ${size} -h ${size} -f png -o ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${size}x${size}/apps/libresplit.png libresplit.svg)")
    install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${size}x${size}/apps DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${size}x${size}/apps)
endforeach()

# Icon cache update rule (optional)
if(NOT DESTDIR)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -t ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor)")
endif()

# Install gschema
install(FILES libresplit.gschema.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas)

# Compile schemas
if(NOT DESTDIR)
    install(CODE "execute_process(COMMAND glib-compile-schemas ${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas)")
endif()

# Uninstallation target (custom)
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_INSTALL_PREFIX}/bin/libresplit
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_INSTALL_PREFIX}/share/applications/libresplit.desktop
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor
)

# Formatting target using clang-format
add_custom_target(format
    COMMAND clang-format -i ${SOURCES} ${COMPONENTS} ${SRC_DIR}/*.h ${COMPONENT_DIR}/*.h
)

