# ntsc-rs-obs
[ntsc-rs](https://github.com/valadaptive/ntsc-rs) as an obs video filter plugin.

This plugin consists of a bare-minimum C binding for ntsc-rs (in the `ntscrs-cbind` directory)
and an obs plugin that uses it to apply the filter to a video source.

### Warning
This effect is quite CPU intensive: it's recommended to resize sources to 480p before processing them.

## Supported Build Environments

| Platform  | Tool   |
|-----------|--------|
| Windows   | Visal Studio 17 2022 |
| macOS     | XCode 16.0 |
| Windows, macOS  | CMake 3.30.5 |
| Ubuntu 24.04 | CMake 3.28.3 |
| Ubuntu 24.04 | `ninja-build` |
| Ubuntu 24.04 | `pkg-config`
| Ubuntu 24.04 | `build-essential` |

## Building
These instructions are adapted from the [obs-plugintemplate wiki](https://github.com/obsproject/obs-plugintemplate/wiki/Quick-Start-Guide).
Make sure you have Rust, CMake, and obs-studio/libobs installed for your platform.

### Linux
Run one of the following in the repo's root directory depending on your setup:
```bash
cmake --preset linux        # generate build files for ninja
cmake --preset linux-make   # ditto but for make
```

Then build and package with:
```bash
cmake --build --preset linux
cmake --install build_x86_64 --prefix release-linux
```

### Windows
Ensure you have the most recent version of Visual Studio 17 2022 installed. Then run the following in the repo's root directory:
```bash
cmake --preset windows-x64
cmake --build --preset windows-x64
cmake --install build_x64 --prefix release-windows
```

### macOS
Ensure you have the latest version of Xcode installed lmao, then run the following in the repo's root directory:
```bash
cmake --preset macos
cmake --build --preset macos
cmake --install build_macos --prefix release-macos
```

## GitHub Actions & CI
This repo has a bunch of CI batteries included from [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate);
all of it is documented there.
