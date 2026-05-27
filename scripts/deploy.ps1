param(
    [string]$Kind = "power",
    [string]$Version = "v0.1.0",
    [string]$ServerUser = "root",
    [string]$ServerHost = "38.76.214.157",
    [string]$ServerPort = "22",
    [string]$RemoteStorage = "/var/www/update.hepi.ng/storage",
    [string]$SshKey = "$env:USERPROFILE\.ssh\f4cp_deploy_ed25519"
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
        "发布说明：测试固件" `
        "适用设备：测试固件" `
        "更新内容：测试固件" `
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