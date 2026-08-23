param(
    [Parameter(Mandatory = $true)]
    [string]$ReplayExecutable,

    [Parameter(Mandatory = $true)]
    [string]$StrategyExecutable,

    [Parameter(Mandatory = $true)]
    [string]$FixturePath,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"

function Assert-Contains {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$Expected,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    if (-not $Text.Contains($Expected)) {
        throw "$Context did not contain expected text: $Expected`nActual output:`n$Text"
    }
}

function Assert-NotContains {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$Unexpected,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    if ($Text.Contains($Unexpected)) {
        throw "$Context unexpectedly contained text: $Unexpected`nActual output:`n$Text"
    }
}

function Invoke-BookforgeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    $output = & $Executable @Arguments 2>&1 | Out-String

    if ($LASTEXITCODE -ne 0) {
        throw "$Context failed with exit code $LASTEXITCODE.`nOutput:`n$output"
    }

    return $output
}

if (-not (Test-Path -LiteralPath $ReplayExecutable -PathType Leaf)) {
    throw "Replay executable was not found: $ReplayExecutable"
}

if (-not (Test-Path -LiteralPath $StrategyExecutable -PathType Leaf)) {
    throw "Strategy executable was not found: $StrategyExecutable"
}

if (-not (Test-Path -LiteralPath $FixturePath -PathType Leaf)) {
    throw "Fixture was not found: $FixturePath"
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$replayAllOutput = Invoke-BookforgeCommand `
    -Executable $ReplayExecutable `
    -Arguments @($FixturePath) `
    -Context "All-symbol replay"

Assert-Contains -Text $replayAllOutput -Expected "Input events: 6" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Replayed events: 6" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Symbols: 2" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Symbol: BTCUSDT.P" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Symbol: ETHUSDT.P" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Final best bid: 99" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Final best ask: 100" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Final best bid: 89" `
    -Context "All-symbol replay"
Assert-Contains -Text $replayAllOutput -Expected "Final best ask: 90" `
    -Context "All-symbol replay"

$btcReplayOutput = Invoke-BookforgeCommand `
    -Executable $ReplayExecutable `
    -Arguments @($FixturePath, "--symbol", "BTCUSDT.P") `
    -Context "BTC-only replay"

Assert-Contains -Text $btcReplayOutput -Expected "Symbol filter: BTCUSDT.P" `
    -Context "BTC-only replay"
Assert-Contains -Text $btcReplayOutput -Expected "Input events: 6" `
    -Context "BTC-only replay"
Assert-Contains -Text $btcReplayOutput -Expected "Replayed events: 3" `
    -Context "BTC-only replay"
Assert-Contains -Text $btcReplayOutput -Expected "Symbols: 1" `
    -Context "BTC-only replay"
Assert-Contains -Text $btcReplayOutput -Expected "Symbol: BTCUSDT.P" `
    -Context "BTC-only replay"
Assert-NotContains -Text $btcReplayOutput -Unexpected "Symbol: ETHUSDT.P" `
    -Context "BTC-only replay"
Assert-Contains -Text $btcReplayOutput -Expected "Final best bid: 99" `
    -Context "BTC-only replay"
Assert-Contains -Text $btcReplayOutput -Expected "Final best ask: 100" `
    -Context "BTC-only replay"

$experimentOutputPath = Join-Path $OutputDirectory "btc_fixture_experiment.csv"
Remove-Item -LiteralPath $experimentOutputPath -Force -ErrorAction SilentlyContinue

$strategyOutput = Invoke-BookforgeCommand `
    -Executable $StrategyExecutable `
    -Arguments @(
        "--input",
        $FixturePath,
        "--output",
        $experimentOutputPath,
        "--symbol",
        "BTCUSDT.P",
        "--mode",
        "aggressive",
        "--side",
        "buy",
        "--limit-price",
        "101",
        "--quantity",
        "2",
        "--entry-offset",
        "1"
    ) `
    -Context "BTC strategy experiment"

Assert-Contains -Text $strategyOutput -Expected "Symbol filter: BTCUSDT.P" `
    -Context "BTC strategy experiment"
Assert-Contains -Text $strategyOutput -Expected "Input events: 6" `
    -Context "BTC strategy experiment"
Assert-Contains -Text $strategyOutput -Expected "Replayed events: 3" `
    -Context "BTC strategy experiment"
Assert-Contains -Text $strategyOutput -Expected "Filled quantity: 2" `
    -Context "BTC strategy experiment"
Assert-Contains -Text $strategyOutput -Expected "Remaining quantity: 0" `
    -Context "BTC strategy experiment"
Assert-Contains -Text $strategyOutput -Expected "Average execution price: 100" `
    -Context "BTC strategy experiment"

if (-not (Test-Path -LiteralPath $experimentOutputPath -PathType Leaf)) {
    throw "Strategy experiment did not create result CSV: $experimentOutputPath"
}

$csvLines = Get-Content -LiteralPath $experimentOutputPath

if ($csvLines.Count -ne 2) {
    throw "Expected strategy result CSV to contain one header and one data row. Found $($csvLines.Count) lines."
}

Assert-Contains -Text $csvLines[0] -Expected "requested_qty,filled_qty,remaining_qty" `
    -Context "Strategy result CSV header"
Assert-Contains -Text $csvLines[1] -Expected '"aggressive",1,true,101,2,2,0,1,100' `
    -Context "Strategy result CSV row"

Remove-Item -LiteralPath $experimentOutputPath -Force

Write-Output "Bookforge replay CLI integration test passed."
