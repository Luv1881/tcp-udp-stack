add_library(core STATIC EventLoop.cpp Stack.cpp)
target_link_libraries(core PUBLIC net link common)
