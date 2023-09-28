<p align="center">
  <img src="./Docs/img/icon.png" alt="logo" width="200" />
</p>

# Project64

Project64 is a free and open-source emulator for the Nintendo 64 and Nintendo 64 Disk Drive written in C++ currently only for Windows (planned support for other platforms in the future).

## Features

- Development and debugging tools
- Save/load states
- Fullscreen
- Controller support
- Great language support
- Support for many popular N64 emulator plugins

## Screenshot

<p align="center">
  <img src="./Docs/img/screen.png" alt="screenshot" width="400" />
</p>

## Installation

Installer for the latest stable releases are available [here](https://www.pj64-emu.com/windows-downloads).

Download nightly builds [here](https://www.pj64-emu.com/nightly-builds).

If you want to contribute to this project, please click [here](https://github.com/project64/project64/blob/develop/Docs/BUILDING.md) to get more infromation on how to set up a local build environment.

AppVeyor (Windows x86/x64): [![Build status](https://ci.appveyor.com/api/projects/status/sbtwyhaexslyhgx3?svg=true
)](https://ci.appveyor.com/project/project64/project64/branch/develop)

*Side note: 64-bit builds are considered experimental and aren't currently supported*

## Minimum requirements

* Operating system (limited support for Windows 8.1 and below)
  * Windows XP SP3, Windows 7 SP1, Windows 8.1, and the latest version of Windows 10
* CPU
  * Intel or AMD processor with at least SSE2 support
* RAM
  * 512MB or more
* Graphics card
  * DirectX 8 capable (Jabo's Direct3D8)
  * OpenGL 2.0 capable (3.0+ recommended) (Project64 Video)
  * OpenGL 3.3 capable (GLideN64)

<sub>Intel integrated graphics can have issues that are not present with Nvidia and AMD GPU's even when the requirements are met.</sub>

## Support

For support, we ask all users read our [support document](./Docs/SUPPORT.md). Read this ***before*** opening issues.

Please join our [Discord server](https://discord.gg/Cg3zquF) for support, questions, etc.

## Changelog

If you would like to see a changelog that is available [here](./Docs/CHANGELOG.md).

## Dependencies

- [Duktape](https://duktape.org/): MIT license
- [7-Zip](https://7-zip.org/): LGPL+unRAR license
- [zlib](https://zlib.net/): zlib license
- [libpng](http://libpng.org/pub/png/libpng.html): libpng license
- [discord-rpc](https://github.com/discord/discord-rpc): MIT license
- DirectX: Copyright (C) Microsoft
- [Windows Template Library](https://wtl.sourceforge.io/): Common Public License

## Contributing

Contributions are always welcome!

See the [contributing](./.github/CONTRIBUTING.md) file for ways to get started.

## Maintainers and contributors

- [@Project64](https://www.github.com/project64) - Zilmar - current maintainer
- Jabo - Previous contributor
- Smiff - Previous contributor
- Gent - Previous contributor

Also see the list of [community contributors](https://github.com/project64/project64/contributors).

## 🔗 Links
- [Website](https://pj64-emu.com)
- [Discord](https://discord.gg/Cg3zquF)

## License

![GitHub](https://img.shields.io/github/license/project64/project64)

Please see the [license](./license.md) for more details.
