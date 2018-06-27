@echo off

set rootpath=%~dp0

rem Build libdev
cd %rootpath%\Dump\Src\libdev
msbuild LibDev.sln /p:Configuration=Release /p:Platform=x64

mkdir dist
xcopy /E /Y /I /Q Include dist\Include
xcopy /E /Y /I /Q x64 dist\x64

rem Build Directory Crawler
cd %rootpath%\Dump\Src\DirectoryCrawler
xcopy /E /Y /I /Q %rootpath%\Dump\Src\libdev\dist libdev\

msbuild DirectoryCrawler.sln /p:Configuration=Release /p:Platform=x64
copy x64\Release\DirectoryCrawler.exe %rootpath%\Dump\bin\

rem Build AceFilter
cd %rootpath%\Dump\Src\AceFilter
xcopy /E /Y /I /Q %rootpath%\Dump\Src\libdev\dist libdev\

msbuild AceFilter.sln /p:Configuration=Release /p:Platform=x64

rem Build ControlRelationsProvider
cd %rootpath%\Dump\Src\ControlRelationsProviders
xcopy /E /Y /I /Q %rootpath%\Dump\Src\libdev\dist libdev\

msbuild ControlRelationsProviders.sln /p:Configuration=Release /p:Platform=x64 /m

rem End build
cd %rootpath%
