#include "core/Stack.h"
#include <cstdio>

int main() {
    core::Stack stack("tun0", 0x0A000002u);
    std::puts("[phase2] stack up — 10.0.0.2 via tun0");
    stack.run();
}
