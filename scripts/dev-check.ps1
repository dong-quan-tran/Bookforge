param(
  [string]$ClangFormat = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe",
  [string]$BuildDir = "build",
  [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$files = Get-ChildItem -Path src, tests, bench -Recurse -Include *.cpp, *.hpp, *.h |
  Select-Object -ExpandProperty FullName

if ($files.Count -gt 0) {
  & $ClangFormat -i -style=file @files
}

cmake -S . -B $BuildDir
cmake --build $BuildDir --config $Config
ctest --test-dir $BuildDir -C $Config --output-on-failure