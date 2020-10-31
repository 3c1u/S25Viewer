# S25Viewer

## Build

### Prerequisites

* Stable release of Rust compiler
    * Tested on rustc 1.46.0 (04488afe3 2020-08-24)
* Qt 5.15 or later (Qt 6 supported)
* Cargo
* CMake
* Ninja (optional)

### Instructions

```console
mkdir build
cd build
cmake -GNinja ..
cargo --release
ninja
```

## License

Copyright (c) 2020 Hikaru Terazono (3c1u). All rights reserved.

Licensed under GPLv3.
