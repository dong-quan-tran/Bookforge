param(
  [string]$ClangFormat = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"
)

$ErrorActionPreference = "Stop"

$files = Get-ChildItem -Path src, tests, bench -Recurse -Include *.cpp, *.hpp, *.h |
  Select-Object -ExpandProperty FullName

if ($files.Count -gt 0) {
  & $ClangFormat -i -style=file @files
}

git add -- src tests bench