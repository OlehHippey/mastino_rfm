# encrypt.ps1 — PowerShell скрипт для шифрування прошивки ESP32 + версія з CMakeLists.txt

# Шляхи до файлів
$Tool = "components\espressif__esp_encrypted_img\tools\esp_enc_img_gen.py"
$Input = "build\mastino_hub.bin"
$Output = "build\mastino_hub.enc.bin"
$Key = "key_tsall.pem"

# Виклик Python-скрипта
python $Tool encrypt $Input $Key $Output

# Зчитування версії з CMakeLists.txt
$CMakeText = Get-Content CMakeLists.txt
foreach ($line in $CMakeText) {
    if ($line -match 'set\s*\(\s*PROJECT_VER\s+"([^"]+)"\s*\)') {
        $ver = $matches[1]
        break
    }
}

if (-not $ver) {
    Write-Host "❌ Не знайдено PROJECT_VER у CMakeLists.txt"
    exit 1
}

# Генерація файлу last
$LastPath = "build\last"
Set-Content -Encoding ASCII -Path $LastPath -Value $ver
Write-Host "`n✅ DONE! OTA version file generated: $ver"

# Надсилаємо файл OTA (firmware) на сервер
Write-Host "`n📤 Uploading encrypted firmware..."
Invoke-RestMethod -Uri "http://localhost:8080/upload" -Method Post -InFile "build\mastino_hub.enc.bin" -ContentType "multipart/form-data" -Form @{ file = Get-Item "build\mastino_hub.enc.bin" }

# Надсилаємо файл версії
Invoke-RestMethod -Uri "http://localhost:8080/upload" -Method Post -InFile "build\last" -ContentType "multipart/form-data" -Form @{ file = Get-Item "build\last" }

Write-Host "Upload complete."

