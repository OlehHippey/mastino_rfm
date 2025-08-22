# encrypt.ps1 — PowerShell скрипт для шифрування прошивки ESP32

# Шляхи до файлів
$Tool = "components\espressif__esp_encrypted_img\tools\esp_enc_img_gen.py"
$Input = "build\mastino_hub.bin"
$Output = "build\mastino_hub.enc.bin"
$Key = "key_tsall.pem"

# Виклик Python-скрипта
python $Tool encrypt $Input $Key $Output
