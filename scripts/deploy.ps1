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
        "推荐版发布 $Version",
        "适用设备：F4CP-POWER / STM32G474CBT6",
        "优化 Buck/Mix/Boost 模式状态机，使用实际输出电压判定并加入模式回差，降低临界区反复切换",
        "改进 Buck 到 Mix 的无扰切换，Boost 支路与 PID 种子从当前硬件状态起步，避免理论占空突跳",
        "为 Mix 区加入独立补偿器初值、专用输出限幅与 anti-windup，避免沿用纯 Boost 状态",
        "为电压环采样加入轻量 IIR 滤波，并限制 u0/u1 内部状态，减少 ADC 抖动和隐藏积分跑飞",
        "优化电流环限流状态机，加入 CC/CV 滞回、释放保持时间和更温和的 Vref 拉低/释放步进",
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
