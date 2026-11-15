# Configure
cmake -S . -B build -G Ninja \
    -DCMAKE_PREFIX_PATH="$PWD/.qt/6.12.0/gcc_64" \
    -DCMAKE_BUILD_TYPE=Debug

# Compile
cmake --build build --parallel