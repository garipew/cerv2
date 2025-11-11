# Cerver
This is my second iteration over a minimal HTTP server in C.

My main motivation to tackle this project again was to understand the
limitations and at the same time improve my arena implementation and
also to develop a simpler, easier to read version.

Check by yourself if this codebase is simpler than [the old version](https://github.com/garipew/cerver).

## Build
Simply
```
make
```

## Options
Currently there is only one option implemented.
```
./cerver --root=webroot
```
Sets the root of the server to 'webroot' directory.

When this option is not specified, the root of the server is the current working
directory at the execution.
