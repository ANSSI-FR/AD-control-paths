# Building AD-Control-Paths

## Supported compilers
* Visual Studio 2017 with any Windows 10 SDK

## Supported platforms
* x86 (platform Win32)
* x86_64 (platform x64)

## Build AD-Control-Paths
### Automatic
1. Get all submodules (`git submodule update --init --recursive`)
2. Open a Visual Studio Command Prompt (x64 Native)
3. Run build.bat (+ Grab coffee)

### Manual
1. Get all submodules (`git submodule update --init --recursive`)
2. Build ADCP-libdev or get binary release
3. Put ADCP-libdev dist folders (`Include`, `Debug`, `Release`, `x64`) to `Dump\Src\DirectoryCrawler\libdev`, `Dump\Src\AceFilter\libdev` and `Dump\Src\ControlRelationsProviders\libdev`.
4. Build DirectoryCrawler (or get binary release)
5. Copy `DirectoryCrawler.exe` and `json\ADng_ADCP.json` to `Dump\Bin`
6. Build AceFilter with msbuild or via Visual Studio
7. Build ControlRelationsProvider with msbuild or via Visual Studio


All commands are in `build.bat`.
