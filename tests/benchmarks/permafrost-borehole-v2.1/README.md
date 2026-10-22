# Permafrost borehole compatibility benchmark

This benchmark builds the official iLand 2.1 console from
`tests/iland-official-source-v2.1` with Qt,
builds this repository's `ILandModel` library and console app, runs both against
separate disposable copies of
`tests/permafrost-borehole-benchmark/model`, then compares every output database
table using the reusable comparator in `tests/common/sqlite_compare.c`. The
source model directory and its stored reference files are left unchanged.

The shared Qt build helper is `tests/cmake/BuildOfficial.cmake`; this folder's
`CMakeLists.txt` and `RunBenchmark.cmake` contain the permafrost-specific setup.

Both versions use the same compiler and C++ standard library. On Windows with
the Qt 6.8.1 MinGW kit, configure from the repository root with:

```powershell
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.8.1\mingw_64\bin;$env:PATH"
cmake -S tests/benchmarks/permafrost-borehole-v2.1 -B tests/build -G Ninja -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DILAND_QT_ROOT=C:/Qt/6.8.1/mingw_64 -DILAND_QMAKE=C:/Qt/6.8.1/mingw_64/bin/qmake.exe -DILAND_QT_MAKE=C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe -DILAND_QT_COMPILER_BIN=C:/Qt/Tools/mingw1310_64/bin
cmake --build tests/build --parallel 8
ctest --test-dir tests/build --output-on-failure
```

The build tree contains the copied official source and build products under
`tests/build/official-src`. Run inputs, logs, and output databases are under
`tests/build/runs/official-2.1` and `tests/build/runs/workspace-library`. The
runner sets the random seed to `314159` and disables multithreading in those
copies for repeatable comparisons. It removes old generated outputs only from
the copies before each run.

Numeric database values are compared with a relative and absolute tolerance of
`1e-9`. The build timestamp and version label in the `runinfo` metadata table
are ignored because they describe the two different builds rather than model
results.
