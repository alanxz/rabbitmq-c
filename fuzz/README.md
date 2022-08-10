#### OSS-Fuzz in House
```
export CC=clang
export CXX=clang++
export CFLAGS=-fsanitize=fuzzer-no-link,address
export LIB_FUZZING_ENGINE=-fsanitize=fuzzer
export LDFLAGS=-fsanitize=address 
```
```
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_OSSFUZZ=ON \
-DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX \
-DCMAKE_C_FLAGS=$CFLAGS -DCMAKE_EXE_LINKER_FLAGS=$LDFLAGS \
-DLIB_FUZZING_ENGINE=$LIB_FUZZING_ENGINE \
../
```
```
./fuzz/fuzz_url
./fuzz/fuzz_table
./fuzz/fuzz_server
```
