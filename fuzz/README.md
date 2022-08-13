### OSS-Fuzz in House

#### Export Flags
```
export CC=clang
export CXX=clang++
export CFLAGS=-fsanitize=fuzzer-no-link,address
export LIB_FUZZING_ENGINE=-fsanitize=fuzzer
export LDFLAGS=-fsanitize=address 
```

#### Build cmake Fuzzer
```
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_OSSFUZZ=ON \
-DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX \
-DCMAKE_C_FLAGS=$CFLAGS -DCMAKE_EXE_LINKER_FLAGS=$CFLAGS \
-DLIB_FUZZING_ENGINE=$LIB_FUZZING_ENGINE \
../
```

#### Run Fuzzer
```
mkdir coverage
./fuzz/fuzz_url coverage/ ../fuzz/input/
./fuzz/fuzz_table coverage/ ../fuzz/input/
./fuzz/fuzz_server coverage/ ../fuzz/input/
```
