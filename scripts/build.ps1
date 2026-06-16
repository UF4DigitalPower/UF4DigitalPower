param(
    [string]$BuildDir = "E:\PROJECT_C\UF4DigitalPower\cmake-build-debug",
    [string]$Jobs = "4"
)

$ErrorActionPreference = "Continue"

cmake --build $BuildDir -- "-j$Jobs"
$buildExitCode = $LASTEXITCODE

Write-Host ""
if ($buildExitCode -eq 0) {
    Write-Host "######     ###     ######   ######" -ForegroundColor Green
    Write-Host "##   ##   ## ##   ##       ##    " -ForegroundColor Green
    Write-Host "######   #######   #####    #####" -ForegroundColor Green
    Write-Host "##       ##   ##       ##       ##" -ForegroundColor Green
    Write-Host "##       ##   ##  ######   ######" -ForegroundColor Green
} else {
    Write-Host "#######  #######  ##       #######  ######" -ForegroundColor Red
    Write-Host "##          ##     ##       ##       ##   ##" -ForegroundColor Red
    Write-Host "#####       ##     ##       #####    ##   ##" -ForegroundColor Red
    Write-Host "##          ##     ##       ##       ##   ##" -ForegroundColor Red
    Write-Host "##       #######  #######  #######  ######" -ForegroundColor Red
}
Write-Host ""

exit $buildExitCode
