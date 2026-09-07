param(
    [string]$BuildDir = "build-bench",
    [string]$OutputDir = "output\benchmarks",
    [int]$Repetitions = 7,
    [double]$MinTimeSeconds = 2.0,
    [switch]$SkipConfigure
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

function Find-BenchmarkExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [string]$SearchRoot
    )

    $matches = @(
        Get-ChildItem -Path $SearchRoot -Recurse -File -Filter "$Name.exe" |
            Select-Object -ExpandProperty FullName
    )

    if ($matches.Count -eq 0) {
        throw "Could not find $Name.exe under '$SearchRoot'."
    }

    return $matches[0]
}

if ($Repetitions -lt 3) {
    throw "Repetitions must be at least 3."
}

if ($MinTimeSeconds -le 0) {
    throw "MinTimeSeconds must be positive."
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

if (-not $SkipConfigure) {
    cmake -S . -B $BuildDir `
        -DCMAKE_BUILD_TYPE=Release `
        -DBOOKFORGE_ENABLE_BENCHMARKS=ON
    Assert-LastExitCode "CMake benchmark configuration"
}

cmake --build $BuildDir --config Release --parallel
Assert-LastExitCode "CMake benchmark build"

$replayExecutable = Find-BenchmarkExecutable "benchmark_replay" $BuildDir
$orderBookExecutable = Find-BenchmarkExecutable "benchmark_order_book" $BuildDir

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$replayOutput = Join-Path $OutputDir "replay-$timestamp.json"
$orderBookOutput = Join-Path $OutputDir "order-book-$timestamp.json"
$metadataOutput = Join-Path $OutputDir "run-$timestamp.md"

& $replayExecutable `
    "--benchmark_min_time=$MinTimeSeconds" `
    "--benchmark_repetitions=$Repetitions" `
    "--benchmark_report_aggregates_only=true" `
    "--benchmark_format=json" `
    "--benchmark_out=$replayOutput"
Assert-LastExitCode "Replay benchmark"

& $orderBookExecutable `
    "--benchmark_min_time=$MinTimeSeconds" `
    "--benchmark_repetitions=$Repetitions" `
    "--benchmark_report_aggregates_only=true" `
    "--benchmark_format=json" `
    "--benchmark_out=$orderBookOutput"
Assert-LastExitCode "Order-book benchmark"

$commit = git rev-parse HEAD
Assert-LastExitCode "Git commit lookup"

$branch = git branch --show-current
Assert-LastExitCode "Git branch lookup"

$cmakeVersion = cmake --version | Select-Object -First 1
$pythonVersion = python --version
$osVersion = [System.Environment]::OSVersion.VersionString
$processor = $env:PROCESSOR_IDENTIFIER
$logicalProcessors = [Environment]::ProcessorCount
$fixturePath = "tests/fixtures/hyperliquid_replay_fixture_large.csv"
$fixtureLineCount = (Get-Content -LiteralPath $fixturePath | Measure-Object -Line).Lines
$fixtureEventCount = $fixtureLineCount - 1

@"
# Benchmark Run

- Collected at: $(Get-Date -Format "yyyy-MM-ddTHH:mm:ssK")
- Git commit: $commit
- Git branch: $branch
- Operating system: $osVersion
- Processor: $processor
- Logical processors: $logicalProcessors
- CMake: $cmakeVersion
- Python: $pythonVersion
- Build directory: $BuildDir
- Build type: Release
- Google Benchmark repetitions: $Repetitions
- Minimum benchmark duration per repetition: $MinTimeSeconds seconds
- Replay fixture: $fixturePath
- Replay fixture events: $fixtureEventCount
- Replay fixture mix: 8,334 New events and 1,666 Reject events for the default 10,000-event fixture
- Replay benchmark scope: in-memory events replayed through ReplayRunner, HyperliquidMatchingEngineAdapter, MatchingEngine, and OrderBook. CSV file I/O and parsing occur before timed iterations.
- Order-book isolated benchmark scope: setup is paused; the timed region contains only the named operation against a prepopulated book.
- Order-book workload benchmark scope: setup and mutation phases are both timed intentionally.
- Replay JSON: $replayOutput
- Order-book JSON: $orderBookOutput
"@ | Set-Content -LiteralPath $metadataOutput -Encoding utf8

$replayResults = (Get-Content -LiteralPath $replayOutput -Raw | ConvertFrom-Json).benchmarks
$orderBookResults = (Get-Content -LiteralPath $orderBookOutput -Raw | ConvertFrom-Json).benchmarks

Write-Output ""
Write-Output "===== REPLAY BENCHMARK AGGREGATES ====="
$replayResults |
    Where-Object { $_.aggregate_name -in @("mean", "median", "stddev", "cv") } |
    Select-Object name, aggregate_name, real_time, cpu_time, time_unit, items_per_second |
    Format-List

Write-Output ""
Write-Output "===== ORDER-BOOK MEDIAN RESULTS ====="
$orderBookResults |
    Where-Object { $_.aggregate_name -eq "median" } |
    Select-Object name, real_time, cpu_time, time_unit, items_per_second |
    Format-Table -AutoSize

Write-Output ""
Write-Output "Benchmark artifacts written to:"
Write-Output "  $metadataOutput"
Write-Output "  $replayOutput"
Write-Output "  $orderBookOutput"
