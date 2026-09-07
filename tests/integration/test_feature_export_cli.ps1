param(
    [Parameter(Mandatory = $true)]
    [string]$FeatureExportExecutable,

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

function Assert-Equal {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Actual,

        [Parameter(Mandatory = $true)]
        [object]$Expected,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    if ($Actual -ne $Expected) {
        throw "$Context expected '$Expected' but got '$Actual'."
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

if (-not (Test-Path -LiteralPath $FeatureExportExecutable -PathType Leaf)) {
    throw "Feature export executable was not found: $FeatureExportExecutable"
}

if (-not (Test-Path -LiteralPath $FixturePath -PathType Leaf)) {
    throw "Fixture was not found: $FixturePath"
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$outputPath = Join-Path $OutputDirectory "btc_multi_symbol_features.csv"
Remove-Item -LiteralPath $outputPath -Force -ErrorAction SilentlyContinue

$output = Invoke-BookforgeCommand `
    -Executable $FeatureExportExecutable `
    -Arguments @(
        "--input",
        $FixturePath,
        "--output",
        $outputPath,
        "--symbol",
        "BTCUSDT.P",
        "--snapshot-depth",
        "2",
        "--imbalance-depth",
        "2",
        "--ofi-depth",
        "2",
        "--rolling-window",
        "2",
        "--strict",
        "--log-every",
        "0"
    ) `
    -Context "BTC-only feature export"

Assert-Contains -Text $output -Expected "[feature_export] symbol_filter=BTCUSDT.P" `
    -Context "BTC-only feature export"
Assert-Contains -Text $output -Expected "[feature_export] input_events=6" `
    -Context "BTC-only feature export"
Assert-Contains -Text $output -Expected "[feature_export] filtered_events=3" `
    -Context "BTC-only feature export"
Assert-Contains -Text $output -Expected "[feature_export] wrote_rows=3" `
    -Context "BTC-only feature export"

if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) {
    throw "Feature export did not create output CSV: $outputPath"
}

$rows = Import-Csv -LiteralPath $outputPath

Assert-Equal -Actual $rows.Count -Expected 3 -Context "Feature row count"

foreach ($row in $rows) {
    Assert-Equal -Actual $row.symbol -Expected "BTCUSDT.P" -Context "Feature row symbol"
}

Assert-Equal -Actual $rows[0].replay_event_index -Expected "1" `
    -Context "First replay event index"
Assert-Equal -Actual $rows[1].replay_event_index -Expected "2" `
    -Context "Second replay event index"
Assert-Equal -Actual $rows[2].replay_event_index -Expected "3" `
    -Context "Third replay event index"

Assert-Equal -Actual $rows[0].replay_timestamp_ns -Expected "1765800000000000000" `
    -Context "First replay timestamp"
Assert-Equal -Actual $rows[1].replay_timestamp_ns -Expected "1765800000000000200" `
    -Context "Second replay timestamp"
Assert-Equal -Actual $rows[2].replay_timestamp_ns -Expected "1765800000000000400" `
    -Context "Third replay timestamp"

Assert-Equal -Actual $rows[2].best_bid -Expected "99" `
    -Context "Final BTC best bid"
Assert-Equal -Actual $rows[2].best_ask -Expected "100" `
    -Context "Final BTC best ask"
Assert-Equal -Actual $rows[2].spread -Expected "1" `
    -Context "Final BTC spread"
Assert-Equal -Actual $rows[2].mid_price -Expected "99.5" `
    -Context "Final BTC mid price"

Remove-Item -LiteralPath $outputPath -Force

Write-Output "Bookforge feature export CLI integration test passed."