#!/bin/bash

builddir="build"
exeName="exe.elf"
#remove old build bc reasons
rm -r ${builddir}

# Generate build files with CMake
cmake -B ${builddir}

# Build the project
cmake --build ${builddir}

echo -e "\n\n\n"

./${builddir}/${exeName}
