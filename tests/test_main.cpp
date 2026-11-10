// G-Engine — tests/test_main.cpp
#include "test_framework.h"

int main(int argc, char** argv) {
    return getest::runAll(argc > 1 ? argv[1] : "");
}
