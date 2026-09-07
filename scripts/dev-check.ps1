param(
    [string]$ClangFormat = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe",
    [string]$BuildDir = "build",
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$SkipFormat,
    [switch]$SkipLint,
    [switch]$SkipPython,
    [switch]$EnableBenchmarks
)

$ErrorActionPreference = "Stop"

function Assert-LastExitCode {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Step
    )

    if ($LASTEXITCODE -ne 0) {
        throw "$Step failed with exit code ${LASTEXITCODE}."
    }
}

if (-not $SkipFormat) {
    if (-not (Test-Path -LiteralPath $ClangFormat)) {
        throw "clang-format was not found at '$ClangFormat'. Pass -ClangFormat with the correct path."
    }

    $cppFiles = @(
        Get-ChildItem -Path src, tests, bench -Recurse -File -Include *.cpp, *.hpp, *.h |
            Select-Object -ExpandProperty FullName
    )

    if ($cppFiles.Count -gt 0) {
        & $ClangFormat -i -style=file @cppFiles
        Assert-LastExitCode "C++ formatting"
    }
}

$benchmarkOption = if ($EnableBenchmarks) { "ON" } else { "OFF" }

cmake -S . -B $BuildDir "-DBOOKFORGE_ENABLE_BENCHMARKS=$benchmarkOption"
Assert-LastExitCode "CMake configuration"

cmake --build $BuildDir --config $Config
Assert-LastExitCode "CMake build"

ctest --test-dir $BuildDir -C $Config --output-on-failure
Assert-LastExitCode "CTest"

if (-not $SkipLint) {
    python -m ruff check .
    Assert-LastExitCode "Ruff lint"

    python -m ruff format --check .
    Assert-LastExitCode "Ruff formatting check"
}

if (-not $SkipPython) {
    $previousPythonPath = $env:PYTHONPATH

    try {
        $env:PYTHONPATH = "python"

        python -m pytest tests/python -q
        Assert-LastExitCode "Python tests"

        python -m compileall -q python
        Assert-LastExitCode "Python compilation check"
    }
    finally {
        $env:PYTHONPATH = $previousPythonPath
    }
}

Write-Host "Bookforge development checks passed."
