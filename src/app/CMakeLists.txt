add_executable(phase1 phase1.cpp)
target_link_libraries(phase1 PRIVATE link net common)

add_executable(phase2 phase2.cpp)
target_link_libraries(phase2 PRIVATE core)
