#!/bin/bash

BUILD_TYPE=Debug
BUILD_DIR="build"
ADDITIONAL_BUILD_OPTIONS=""

for var in "$@"; do
	if [ "$var" = "--release" ]; then
		echo "Build type set to 'Release'"
		BUILD_TYPE=Release
	fi
	case "$var" in -D*)
		echo "Passing $var to CMake."
		ADDITIONAL_BUILD_OPTIONS+="$var "
	esac
done

BUILD_OPTIONS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
BUILD_OPTIONS+=" -DENABLE_TESTS=ON"
BUILD_OPTIONS+=" $ADDITIONAL_BUILD_OPTIONS"

cmake -B "$BUILD_DIR" -S . $BUILD_OPTIONS
cmake --build "$BUILD_DIR" -j7
BUILD_RESULT=$?

if [ $BUILD_RESULT -ne 0 ]; then
	echo "Build failed. Skipping tests and post-build steps."
	exit $BUILD_RESULT
fi

ctest --test-dir "$BUILD_DIR"
