# encrypt.ps1 - Encrypt firmware, extract version, upload to OTA server

# Config
$Tool = "components\espressif__esp_encrypted_img\tools\esp_enc_img_gen.py"
$Input = "build\mastino_hub.bin"
$Output = "build\mastino_hub.enc.bin"
$Key = "key_tsall.pem"
$LastPath = "build\last"
$UploadUrl = "http://10.10.8.6:7369/upload"  # Change if needed

# 1. Encrypt firmware
Write-Host "Encrypting firmware..."
python $Tool encrypt $Input $Key $Output
if ($LASTEXITCODE -ne 0) {
    Write-Host "Encryption failed."
    exit 1
}

# 2. Extract version from CMakeLists.txt
Write-Host "Extracting version from CMakeLists.txt..."
$CMakeText = Get-Content CMakeLists.txt
$ver = $null
$regex = 'set\s*\(\s*PROJECT_VER\s+"([^"]+)"\s*\)'

foreach ($line in $CMakeText) {
    if ($line -match $regex) {
        $ver = $matches[1]
        break
    }
}

if (-not $ver) {
    Write-Host "PROJECT_VER not found in CMakeLists.txt"
    exit 1
}
Write-Host "Found version: $ver"

# 3. Create version file
Set-Content -Encoding ASCII -Path $LastPath -Value $ver
Write-Host "Version file created: $LastPath"

# 4. Upload firmware and version using curl
Write-Host "Uploading firmware..."
& curl.exe -s -F "file=@$Output" $UploadUrl

Write-Host "Uploading version file..."
& curl.exe -s -F "file=@$LastPath" $UploadUrl

Write-Host "Done."
