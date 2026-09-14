# Прошивка ESP32-C3-MINI-V1 на Windows 10/11

## Через Arduino IDE 2

1. Установите [Arduino IDE](https://www.arduino.cc/en/software/). Скачайте репозиторий через **Code → Download ZIP** и распакуйте, например в `C:\Projects\esp32-lrf-tf03`.
2. В **File → Preferences → Additional Boards Manager URLs** добавьте:

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. Откройте **Boards Manager**, найдите **esp32 by Espressif Systems** и установите версию **3.3.11** — с ней проверялась сборка проекта.
4. В папке `firmware\lrf_hf_bridge` скопируйте `Secrets.example.h` в `Secrets.h`. Включите показ расширений файлов в Проводнике: файл должен называться `Secrets.h`, не `Secrets.h.txt`. При необходимости поменяйте имя точки доступа и пароль (не менее 8 символов). По умолчанию `lidar_bridge` / `00000000`.
5. Откройте `firmware\lrf_hf_bridge\lrf_hf_bridge.ino`. Остальные файлы скетча должны остаться рядом с ним.
6. Подключите ESP кабелем USB **с передачей данных**. В **Tools** выберите:

   | Настройка | Значение |
   |---|---|
   | Board | ESP32C3 Dev Module |
   | USB CDC On Boot | Enabled |
   | Flash Mode | DIO |
   | Flash Size | 4MB (32Mb) |
   | Partition Scheme | Default 4MB with spiffs |
   | Port | COM-порт вашей ESP |

7. Нажмите **Verify**, затем **Upload**. Закройте программы, занявшие COM-порт. Дождитесь успешной проверки записи и перезапуска.
8. Откройте **Serial Monitor**, скорость **115200**. Ожидается строка вида `TF03 ... cm=... valid=... simulated=0 sent=... baud=115200 TX=10`. Без данных датчика `cm=0 valid=0`, счётчик `sent` продолжает расти.

Прошивка использует встроенный USB ESP32-C3 для диагностики. UART0 GPIO10 занят выходом TF03, не подключайте к нему USB-UART монитор одновременно с полётником. Если COM-порта нет, проверьте кабель и Диспетчер устройств. Для плат с отдельным USB-UART преобразователем драйвер выбирается по его чипу, а не наугад.

## Если загрузка не начинается

Удерживайте **BOOT**, кратко нажмите **RESET/EN**, отпустите **BOOT**. Если кнопки RESET нет, подключите USB при зажатой BOOT. Повторно выберите COM-порт: в загрузчике он может измениться. Запустите Upload; после окончания нажмите RESET или переподключите питание. Если есть внешние провода на загрузочных GPIO8/9, они могут мешать запуску; этот проект их не использует.

## Через PowerShell и Arduino CLI

Установите Windows MSI из [официальных выпусков Arduino CLI](https://github.com/arduino/arduino-cli/releases), откройте новое окно PowerShell и проверьте `arduino-cli version`.

```powershell
cd C:\Projects\esp32-lrf-tf03
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.11 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
Copy-Item firmware\lrf_hf_bridge\Secrets.example.h firmware\lrf_hf_bridge\Secrets.h
arduino-cli board list
```

Команду Copy-Item выполняйте только при первом запуске, чтобы не перезаписать свои настройки. Замените `COM5` ниже на фактический порт:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\flash.ps1 -Port COM5
```

`ExecutionPolicy Bypass` действует только для этого запуска, постоянные настройки Windows не меняются. При корпоративном запрете запуска скриптов используйте Arduino IDE либо прямые команды:

```powershell
arduino-cli compile --fqbn "esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M" --build-path build/cache/c3 --output-dir build/c3 firmware/lrf_hf_bridge
arduino-cli upload --fqbn "esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashMode=dio,FlashSize=4M" --port COM5 --input-dir build/c3 firmware/lrf_hf_bridge
arduino-cli monitor --port COM5 --config baudrate=115200
```

После ошибки compile не выполняйте upload: иначе можно записать старую сборку. Скрипт flash.ps1 проверяет код завершения автоматически.

## Проверка после прошивки

Подключитесь к Wi-Fi `lidar_bridge`, откройте `http://legion.lidar` (резервный адрес `http://192.168.4.1`). После обновления переподключитесь к Wi-Fi для получения DNS. Без USB программа продолжает работать от подходящего внешнего питания. Выход TF03: GPIO10, 115200 8N1, 50 Гц; при потере данных 0 см. Настройка полётника выполняется отдельно. Для реальных измерений в Config.h должно быть `SIMULATE_DISTANCE = false`.

Инструкция сверена с [установкой Arduino-ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) и [описанием меню платы](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/tools_menu.html). Сборка проверена на macOS; физическая прошивка с Windows пока не проверялась.
