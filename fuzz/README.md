#### Libfuzzer
```
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DBUILD_LIBFUZZ=ON ../

./fuzz/fuzz_url
./fuzz/fuzz_table
```

#### AFL Fuzzer
```
cmake -DCMAKE_C_COMPILER=afl-clang-fast -DCMAKE_BUILD_TYPE=Debug -DBUILD_AFLFUZZ=ON ../

afl-fuzz -i afl_in -o afl_out -- ./fuzz_server 8080 @@

```
