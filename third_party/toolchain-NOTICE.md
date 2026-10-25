# Windows toolchain runtime notices

The portable MinGW build statically links its GCC and MinGW-w64 runtime support.
Session report saving uses the C++ standard library's asynchronous execution,
including the toolchain's winpthreads implementation. No separate runtime install
or service is required.

The `toolchain/` directory preserves the runtime license notices supplied with
the compiler used to build this portable package. These notices supplement the
project MIT license and `osu-LICENCE.txt`; they do not replace either notice.
