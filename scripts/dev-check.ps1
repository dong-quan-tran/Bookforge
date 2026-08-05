param(
  [string[]]$Files = @(),
  [string]$ClangFormat = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe",
  [string]$BuildDir = "build",
  [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

if ($Files.Count -gt 0) {
  & $ClangFormat -i @Files
}

cmake -S . -B $BuildDir
cmake --build $BuildDir --config $Config
ctest --test-dir $BuildDir -C $Config --output-on-failure