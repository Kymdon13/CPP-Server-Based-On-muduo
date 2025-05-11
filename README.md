# CppServer

### Intro
Make an easily implemented server with cpp.

### Build

Use the follow:

```shell
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"
```

to configure.

And then run `ninja server` in the `build/` to build the server.

### Format of log file

Below we show the format of the log file.

``` log
[time] <thread id> [log level] (file:line) [function] message\n
2025-04-11 00:30:41.394577 <760813> [TRACE] (async-log-test.cc:16) [main] hello world
```