param(
    [string]$Kind = "power",
    [string]$Version = "v0.1.0",
    [string]$ServerUser = "root",
    [string]$ServerHost = "38.76.214.157",
    [string]$ServerPort = "1564",
    [string]$RemoteStorage = "/var/www/update.hepi.ng/storage",
    [string]$SshKey = "$env:USERPROFILE\.ssh\hepi_deploy_ed25519"
)

Set-StrictMode -Version Latest
$ReleaseDate = Get-Date -Format "yyyy-MM-dd"

uv run python scripts/publish_firmware.py `
    --kind $Kind `
    --version $Version `
    --bin "cmake-build-debug/UF4DigitalPower.bin" `
    --hex "cmake-build-debug/UF4DigitalPower.hex" `
    --channel stable `
    --set-latest `
    --storage "./storage" `
    --notes `
        "正式发布 $Version" `
        "适用设备：F4CP-POWER / STM32G474CBT6" `
        "启用 PB5 栅极驱动器使能输出，上电后默认拉高 GATE_EN" `
        "ADC1 规则采样切换为 VIN/IIN/VOUT/IOUT 四通道，使用 HRTIM 触发和 4x 过采样" `
        "控制环使用快速 ADC 采样值参与 PID，降低输出在 VIN 和 0 之间跳变的风险" `
        "保持 TVLCOM/USB CDC 通信和上下位机电源页协议对接" `
        "发布日期：$ReleaseDate"

if ($LASTEXITCODE -ne 0) {
    Write-Host "publish_firmware.py 执行失败"
    exit 1
}

ssh -p $ServerPort -i $SshKey "${ServerUser}@${ServerHost}" "mkdir -p ${RemoteStorage}/firmware/${Kind}/versions"

if ($LASTEXITCODE -ne 0) {
    Write-Host "远程目录创建失败"
    exit 1
}

scp -P $ServerPort -i $SshKey -r `
    "./storage/firmware/$Kind/versions/$Version" `
    "${ServerUser}@${ServerHost}:${RemoteStorage}/firmware/${Kind}/versions/"

if ($LASTEXITCODE -ne 0) {
    Write-Host "版本目录上传失败"
    exit 1
}

scp -P $ServerPort -i $SshKey `
    "./storage/firmware/$Kind/latest.json" `
    "./storage/firmware/$Kind/index.json" `
    "${ServerUser}@${ServerHost}:${RemoteStorage}/firmware/${Kind}/"

if ($LASTEXITCODE -ne 0) {
    Write-Host "索引文件上传失败"
    exit 1
}

Write-Host "发布完成：$Kind $Version"
