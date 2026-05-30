param(
    [string]$Kind = "power",
    [string]$Version = "v0.0.1",
    [string]$ServerUser = "root",
    [string]$ServerHost = "38.76.214.157",
    [string]$ServerPort = "1564",
    [string]$RemoteStorage = "/var/www/update.hepi.ng/storage",
    [string]$SshKey = "$env:USERPROFILE\.ssh\hepi_deploy_ed25519"
)

$ReleaseDate = Get-Date -Format "yyyy-MM-dd"

python scripts/publish_firmware.py `
    --kind $Kind `
    --version $Version `
    --bin "cmake-build-debug/UF4DigitalPower.bin" `
    --hex "cmake-build-debug/UF4DigitalPower.hex" `
    --channel beta `
    --set-latest `
    --storage "./storage" `
    --notes `
        "本地发布 $Version" `
        "发布说明：完善骨架代码，完善触发逻辑，绑定虚拟串口" `
        "适用设备：F4CP-POWER" `
        "更新内容：添加板级功率测量、最小功率控制和 TVLCOM 协议集成" `
              "- 增加bsp_power.c/h用于ADC结果处理和物理测量转换" `
              "- 添加power_ctrl.C/H，实现最小功率控制状态和设置接口" `
              "- 添加user_tvlcom_protocol.h和user_tvlcom_transport.c/h用于F4CP TVLCOM协议和传输扇入" `
              "- 将TVLCOM的传输和电源控制集成到主固件和USB CDC接收路径中" `
              "- 更新注入通道和协议支持的ADC、DMA、HRTIM和中断配置" `
              "- 更新CMakeLists.txt以包含用户源和头部" `
              "- 添加硬件、测量和控制架构文档 readme.md" `
              "- Add test_user_tvlcom_protocol.C 用于主机端协议测试" `
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