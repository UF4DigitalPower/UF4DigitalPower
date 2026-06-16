param(
    [string]$Kind = "power",
    [string]$Version = "v0.1.0",
    [string]$Channel = "stable",
    [string]$MinClient = "1.0.0",
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
    "--min-client", $MinClient,
    "--storage", "./storage",
    "--notes"
)

if ($Channel -eq "beta") {
    $PublishArgs += @(
        "测试版发布 $Version",
        "适用设备：F4CP-POWER / STM32G474CBT6",
        "修复 Boost/Mix 模式中 BoostDuty 下限钳位错误，避免错误钳位到最大占空比",
        "补强 PID 初始化和模式切换状态复位逻辑，增加 IErr1/i1 清零并恢复 CVCC 模式",
        "修复 OCP 保护停波通道配置错误，统一为当前硬件使用的 TA/TD 通道",
        "修复 Auto_FAN 温度分档判断顺序，确保高温档位能够正确命中",
        "保持当前 LED 状态语义不变，仅维护控制与保护逻辑",
        "发布日期：$ReleaseDate"
    )
}
else {
    $PublishArgs += @(
        "正式发布 $Version",
        "适用设备：F4CP-POWER / STM32G474CBT6",
        "启用 PB5 栅极驱动器使能输出，上电后默认拉高 GATE_EN",
        "ADC1 规则采样切换为 VIN/IIN/VOUT/IOUT 四通道，使用 HRTIM 触发和 4x 过采样",
        "控制环使用快速 ADC 采样值参与 PID，降低输出在 VIN 和 0 之间跳变的风险",
        "保持 UF4/USB CDC 通信和上下位机电源页协议对接",
        "发布日期：$ReleaseDate"
    )
}

if (-not $SkipSetLatest -and $Channel -eq "stable") {
    $PublishArgs += "--set-latest"
}

uv @PublishArgs

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

Write-Host "发布完成：$Kind $Version ($Channel)"
