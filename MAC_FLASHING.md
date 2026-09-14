# Прошивка ESP32-C3-MINI-V1 на macOS

## Двойным щелчком: сборка и прошивка

1. Скачайте архив `esp32-lrf-tf03-source.zip` из GitHub Release и распакуйте его целиком.
2. Подключите ESP кабелем USB с передачей данных, закройте Serial Monitor и другие программы, использующие порт.
3. Откройте `FLASH_MAC.command` двойным щелчком. При первом запуске из скачанного архива может потребоваться правый щелчок → **Открыть** и подтверждение macOS.
4. Скрипт найдёт Arduino CLI 1.5.1 или загрузит его с официального GitHub Arduino с проверкой SHA-256, установит ESP32 core 3.3.11, создаст `Secrets.h` из примера, соберёт код и прошьёт плату. При нескольких USB-портах выберите номер ESP. При первом запуске нужен интернет и свободное место для инструментов; последующие сборки используют установленный core.
5. Дождитесь `SUCCESS`. Подключитесь к `lidar_bridge` / `00000000` и откройте `http://legion.lidar`. Wi-Fi отключится через 5 минут после запуска; на странице есть счётчик. Лидар и TF03 продолжают работать.

Если Finder не разрешает исполнение файла, откройте Terminal, перейдите в распакованную папку и выполните:

```sh
chmod +x FLASH_MAC.command
./FLASH_MAC.command
```

Папку можно перетащить в Terminal после ввода `cd `, чтобы подставить её полный путь. Скрипт поддерживает Apple Silicon и Intel Mac, не требует Homebrew или Arduino IDE. Сохраняет существующий `Secrets.h`. При ошибке сборки загрузка отменяется. При ошибке USB удерживайте BOOT, нажмите RESET, отпустите BOOT и повторите запуск; порт может измениться.

## Arduino IDE

Используйте шаги из [инструкции Arduino IDE](WINDOWS_FLASHING.md#через-arduino-ide-2). На Mac вместо COM выбирайте `/dev/cu.usbmodem…` или USB-UART порт `/dev/cu.usbserial…`; плата **ESP32C3 Dev Module**, USB CDC **Enabled**, Flash Mode **DIO**, Flash Size **4MB**, Partition Scheme **Default 4MB with spiffs**. Откройте скетч `firmware/lrf_hf_bridge/lrf_hf_bridge.ino`, затем Verify → Upload.

## Готовый образ без сборки

Для ESP Flasher скачайте единый образ из Release и следуйте [инструкции по образу](FLASH_IMAGE.md).
