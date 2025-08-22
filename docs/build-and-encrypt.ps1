# build-and-encrypt.ps1 — повний цикл build → encrypt → version

# Параметри версії (вручну або парсити з cfg.h)
$Major = 1
$Minor = 0
$Add   = 3

# Побудова прошивки
Write-Host "🔧 Building project..."
idf.py build
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed."
    exit 1
}

# Шифрування прошивки
Write-Host "🔐 Encrypting firmware..."
.\encrypt.ps1
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Encryption failed."
    exit 1
}

# Генерація версії у файл 'last'
$Version = "$Major.$Minor.$Add"
$LastPath = "build\last"
Set-Content -Encoding ASCII -Path $LastPath -Value $Version
Write-Host "✅ Version file created: $LastPath ($Version)"

# [ОПЦІОНАЛЬНО] Копіювати на сервер (розкоментуйте та адаптуйте):
# $RemotePath = "\\myserver\firmware\ts1\"
# Copy-Item build\mastino_hub.enc.bin $RemotePath
# Copy-Item $LastPath $RemotePath
# Write-Host "🚀 Files copied to server: $RemotePath"

Write-Host "🎉 DONE!"
