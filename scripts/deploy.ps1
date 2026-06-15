param(
    [string]$Kind = "power",
    [string]$Version = "v0.1.0",
    [string]$Channel = "stable",
    [switch]$SkipSetLatest,
    [string]$ServerUser = "root",
    [string]$ServerHost = "38.76.214.157",
    [string]$ServerPort = "1564",
    [string]$RemoteStorage = "/var/www/update.hepi.ng/storage",
    [string]$SshKey = "$env:USERPROFILE\.ssh\hepi_deploy_ed25519"
)

Set-StrictMode -Version Latest
$ReleaseDate = Get-Date -Format "yyyy-MM-dd"

$PublishArgs = @(
    "run", "python", "scripts/publish_firmware.py",
    "--kind", $Kind,
    "--version", $Version,
    "--bin", "cmake-build-debug/UF4DigitalPower.bin",
    "--hex", "cmake-build-debug/UF4DigitalPower.hex",
    "--channel", $Channel,
    "--storage", "./storage",
    "--notes"
)

if ($Channel -eq "beta") {
    $PublishArgs += @(
        "Beta release $Version",
        "Target device: F4CP-POWER / STM32G474CBT6",
        "Fix BoostDuty lower clamp in Boost and Mix modes",
        "Reset PID state more completely in PID_Init and BBModeChange",
        "Fix OCP stop path channels to current hardware TA and TD outputs",
        "Fix Auto_FAN temperature threshold ordering",
        "Keep current LED behavior unchanged",
        "Release date: $ReleaseDate"
    )
}
else {
    $PublishArgs += @(
        "Stable release $Version",
        "Target device: F4CP-POWER / STM32G474CBT6",
        "Enable PB5 gate driver output by default after power-on",
        "Use ADC1 VIN IIN VOUT IOUT sampling with HRTIM trigger and 4x oversampling",
        "Use fast ADC samples in control loop to reduce VIN to 0 output jumps",
        "Keep UF4 and USB CDC communication aligned with host power page",
        "Release date: $ReleaseDate"
    )
}

if (-not $SkipSetLatest -and $Channel -eq "stable") {
    $PublishArgs += "--set-latest"
}

uv @PublishArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "publish_firmware.py failed"
    exit 1
}

ssh -p $ServerPort -i $SshKey "${ServerUser}@${ServerHost}" "mkdir -p ${RemoteStorage}/firmware/${Kind}/versions"

if ($LASTEXITCODE -ne 0) {
    Write-Host "failed to create remote directory"
    exit 1
}

scp -P $ServerPort -i $SshKey -r `
    "./storage/firmware/$Kind/versions/$Version" `
    "${ServerUser}@${ServerHost}:${RemoteStorage}/firmware/${Kind}/versions/"

if ($LASTEXITCODE -ne 0) {
    Write-Host "failed to upload version directory"
    exit 1
}

scp -P $ServerPort -i $SshKey `
    "./storage/firmware/$Kind/latest.json" `
    "./storage/firmware/$Kind/index.json" `
    "${ServerUser}@${ServerHost}:${RemoteStorage}/firmware/${Kind}/"

if ($LASTEXITCODE -ne 0) {
    Write-Host "failed to upload index files"
    exit 1
}

Write-Host "publish complete: $Kind $Version ($Channel)"
