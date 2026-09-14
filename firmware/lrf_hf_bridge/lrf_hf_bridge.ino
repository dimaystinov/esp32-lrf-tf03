#include <Arduino.h>
#include <HardwareSerial.h>
#include "Config.h"
#include "Protocol.h"
#include "Tf03.h"
#include "NetworkBridge.h"
#include <math.h>

#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
#error "Select ESP32C3 Dev Module or ESP32C6 Dev Module"
#endif
#if !ARDUINO_USB_CDC_ON_BOOT
#error "Enable USB CDC On Boot; UART0 is used for TF03"
#endif

HardwareSerial lidar(1);
HardwareSerial autopilot(0);
LrfParser parser;
Measurement measurement;
uint8_t baudIndex = 0, streak = 0;
bool locked = false;
uint32_t scanStart = 0, lastFrame = 0, lastByte = 0;
uint32_t goodFrames = 0, badFrames = 0, rejected = 0, sent = 0, dropped = 0;
DeviceStatus status;

void selectBaud(uint8_t index) {
  baudIndex = index;
  lidar.end();
  lidar.setRxBufferSize(4096);
  lidar.begin(Config::BAUDS[index], SERIAL_8N1, Config::LIDAR_RX, Config::LIDAR_TX);
  parser.reset();
  measurement.valid = false;
  streak = 0;
  locked = false;
  scanStart = millis();
  lastByte = scanStart;
}

bool sendPacket(const uint8_t *data, size_t len) {
  if (autopilot.availableForWrite() < int(len)) { ++dropped; return false; }
  return autopilot.write(data, len) == len;
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  autopilot.setTxBufferSize(512);
  autopilot.begin(Config::FC_BAUD, SERIAL_8N1, Config::FC_RX, Config::FC_TX);
  selectBaud(0);
  NetworkBridge::begin();
}

void loop() {
  uint32_t now = millis();
  if (parser.partial() && uint32_t(now - lastByte) > 20) { parser.reset(); streak = 0; measurement.valid = false; }
  size_t budget = 2048;
  while (budget-- && lidar.available()) {
    uint16_t cm = 0;
    const auto result = parser.feed(uint8_t(lidar.read()), cm);
    lastByte = now;
    if (result == ParseResult::Bad) {
      ++badFrames; streak = 0; measurement.valid = false;
    } else if (result == ParseResult::Good) {
      ++goodFrames; lastFrame = now;
      if (streak < 5) ++streak;
      if (streak >= 5) locked = true;
      measurement.accept(cm, now, Config::MIN_CM, Config::MAX_CM);
      if (!measurement.valid) ++rejected;
    }
  }
  if ((!locked && uint32_t(now - scanStart) >= Config::SCAN_MS) ||
      (locked && uint32_t(now - lastFrame) >= Config::LOST_MS)) {
    selectBaud((baudIndex + 1) % (sizeof(Config::BAUDS) / sizeof(Config::BAUDS[0])));
  }
  const bool healthy = Config::SIMULATE_DISTANCE || (locked && measurement.fresh(now, Config::STALE_MS));
  const uint16_t outputCm = Config::SIMULATE_DISTANCE ? Config::SIMULATED_CM : (healthy ? measurement.cm : 0);
  const uint32_t outputTime = Config::SIMULATE_DISTANCE ? now : measurement.time;
  // TF03 has no measurement acknowledgement. Drain optional RX without claiming acceptance.
  unsigned rxBudget=256;
  while (rxBudget-- && autopilot.available()) { autopilot.read(); ++status.fcRxBytes; }
  static uint32_t lastDistance=0, lastLog=0;
  if (uint32_t(now-lastDistance)>=Config::DISTANCE_PERIOD_MS) {
    lastDistance=now;
    uint8_t packet[9]; encodeTf03(packet,outputCm);
    if (sendPacket(packet,sizeof(packet))) ++sent;
  }
  static uint32_t lastSnapshot=0;
  static uint32_t lastRate=0, previousGood=0;
  if (uint32_t(now-lastRate)>=1000) {
    status.lidarHz=(goodFrames-previousGood)*1000.0f/uint32_t(now-lastRate);
    lastRate=now; previousGood=goodFrames;
  }
  if (uint32_t(now-lastSnapshot)>=20) {
    lastSnapshot=now; status.uptime=now; status.lidarBaud=Config::BAUDS[baudIndex];
    status.sampleTime=outputTime; status.cm=outputCm; status.locked=locked; status.fresh=healthy;
    status.good=goodFrames; status.bad=badFrames; status.rejected=rejected; status.sent=sent; status.txDrop=dropped;
    NetworkBridge::publish(status);
  }
  if (uint32_t(now - lastLog) >= 1000) {
    lastLog = now;
    char text[256];
    const int len = snprintf(text,sizeof(text),
      "TF03 up=%lu cm=%u valid=%u simulated=%u sent=%lu dropped=%lu baud=%lu TX=%d RX_bytes=%lu\n",
      (unsigned long)now,outputCm,healthy,Config::SIMULATE_DISTANCE,(unsigned long)sent,
      (unsigned long)dropped,(unsigned long)Config::FC_BAUD,Config::FC_TX,(unsigned long)status.fcRxBytes);
    if (len > 0 && len < int(sizeof(text)) && Serial && Serial.availableForWrite() >= len)
      Serial.write(reinterpret_cast<const uint8_t *>(text), size_t(len));
    static bool logNetwork=false;
    logNetwork=!logNetwork;
    if (logNetwork) {
      char network[180]; NetworkBridge::summary(network,sizeof(network));
      const size_t n=strlen(network);
      if (Serial && Serial.availableForWrite()>=int(n+1)) { Serial.write(reinterpret_cast<const uint8_t *>(network),n); Serial.write('\n'); }
    }
  }
  delay(1);
}
