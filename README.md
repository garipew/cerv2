# Cerver
> A minimal HTTP server in C.

---

## Build

To build the web server, follow the steps:

```
git clone https://github.com/garipew/cerv2 --recursive
cd cerv2
make
```

## Run localy

To run it locally:

```
./cerver
```


### Options

Currently there is only one option implemented.

```
./cerver --root=webroot
```

Which sets the root of the server to 'webroot' directory.

When this option is not specified, the root of the server is the current working
directory at the execution.
