add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(link)
add_subdirectory(net)
add_subdirectory(app)
