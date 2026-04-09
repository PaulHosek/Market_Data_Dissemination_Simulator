$inputPath = "company_tickers.json"
$outputPath = "tickers.txt"

$json = Get-Content -Raw -Path $inputPath | ConvertFrom-Json

$json.PSObject.Properties | ForEach-Object {
    $_.Value.ticker
} | Set-Content -Path $outputPath

Write-Host "Tickers saved to $outputPath"
