add_library(net STATIC Ipv4Layer.cpp IcmpLayer.cpp)
target_link_libraries(net PUBLIC link common)
