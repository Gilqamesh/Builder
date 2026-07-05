#include "cmake.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>

#include <unistd.h>

#ifndef M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH
# error M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH must be defined by the owning builder
#endif

int main(int argc, char** argv) {
    std::vector<char*> args;
    args.reserve(argc + 1);
    args.push_back(const_cast<char*>(M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH));

    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    args.push_back(nullptr);

    execv(M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH, args.data());
    std::cerr << "execv failed for " << M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_PATH << ": " << std::strerror(errno) << '\n';
    return 127;
}
