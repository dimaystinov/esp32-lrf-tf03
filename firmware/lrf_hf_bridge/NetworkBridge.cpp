#include "NetworkBridge.h"
#include "Config.h"
#include "Secrets.h"
#include "Dashboard.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <lwip/sockets.h>
#include <errno.h>
#include <math.h>
namespace NetworkBridge {
static QueueHandle_t snapshots=nullptr;
static portMUX_TYPE guard=portMUX_INITIALIZER_UNLOCKED;
static char netSummary[150]="network starting";
static DeviceStatus device;
static NetworkServer httpServer(Config::HTTP_PORT);
static NetworkClient httpClient;
static bool apActive=false,serversStarted=false;
static String httpRequest,httpResponse;
static size_t httpOffset=0;
static uint32_t httpStart=0;
static bool fresh(bool seen, uint32_t timestamp, uint32_t now, uint32_t limit) {
  return seen && uint32_t(now - timestamp) < limit;
}
static String num(float value, bool valid, int decimals = 2) {
  return valid && isfinite(value) ? String(value, decimals) : String("null");
}
static const char *boolean(bool value) { return value ? "true" : "false"; }
static String quoted(const String &value) {
  String result = "\"";
  for (unsigned i = 0; i < value.length(); ++i) {
    const uint8_t c = value[i];
    if (c == '"' || c == '\\') { result += '\\'; result += char(c); }
    else if (c >= 32) result += char(c);
  }
  return result + '"';
}
static String lidarJson(uint32_t now) {
  const bool valid = device.fresh && fresh(true, device.sampleTime, now, Config::STALE_MS);
  String s; s.reserve(450);
  s = "{\"locked\":"; s += boolean(device.locked);
  s += ",\"connected\":"; s += boolean(device.locked && device.good > 0 && uint32_t(now - device.sampleTime) < Config::LOST_MS);
  s += ",\"simulated\":"; s += boolean(Config::SIMULATE_DISTANCE);
  s += ",\"valid\":"; s += boolean(valid);
  s += ",\"distance_cm\":"; s += valid ? String(device.cm) : String("0");
  s += ",\"distance_m\":"; s += num(valid ? device.cm * .01f : 0, true);
  s += ",\"age_ms\":"; s += (device.good || Config::SIMULATE_DISTANCE) ? String(uint32_t(now-device.sampleTime)) : String("null");
  s += ",\"baud\":" + String(device.lidarBaud);
  s += ",\"rate_hz\":" + num(device.lidarHz,true,1);
  s += ",\"good\":" + String(device.good) + ",\"bad\":" + String(device.bad);
  s += ",\"rejected\":" + String(device.rejected) + ",\"tf03_sent\":" + String(device.sent);
  s += ",\"min_cm\":" + String(Config::MIN_CM) + ",\"max_cm\":" + String(Config::MAX_CM) + "}";
  return s;
}
static String fcJson(uint32_t) {
  return String("{\"protocol\":\"TF03\",\"connected\":null,\"accepted\":null,\"ack_supported\":false,\"tx_frames\":") + device.sent + ",\"tx_rate_hz\":" + num(device.txHz,true,1) +
    ",\"tx_dropped\":" + device.txDrop + ",\"rx_bytes\":" + device.fcRxBytes + ",\"baud\":" + Config::FC_BAUD + "}";
}
static String statusJson() {
  xQueuePeek(snapshots,&device,0);
  return String("{\"firmware\":\"3.0.0\",\"uptime_ms\":")+millis()+",\"output_protocol\":\"TF03\",\"wifi\":{\"ap_enabled\":"+boolean(apActive)+
    ",\"ssid\":"+quoted(Secrets::AP_SSID)+",\"ap_ip\":"+quoted(WiFi.softAPIP().toString())+",\"clients\":"+WiFi.softAPgetStationNum()+"},\"lidar\":"+lidarJson(millis())+",\"fc\":"+fcJson(millis())+"}";
}
static void serviceWifi() {
  static uint32_t lastAttempt=0;
  if (!apActive && (!lastAttempt || uint32_t(millis()-lastAttempt)>=5000)) {
    lastAttempt=millis(); WiFi.mode(WIFI_AP);
    apActive=WiFi.softAP(Secrets::AP_SSID,Secrets::AP_PASSWORD);
    if (apActive) {
      WiFi.setTxPower(WIFI_POWER_8_5dBm);
      MDNS.begin("lidar-bridge"); MDNS.addService("http","tcp",Config::HTTP_PORT);
    }
  }
  if (apActive && !serversStarted) { httpServer.begin(); serversStarted=true; }
  char text[150];
  snprintf(text,sizeof(text),"AP=%s IP=%s clients=%u",apActive?"ON":"STARTING",WiFi.softAPIP().toString().c_str(),WiFi.softAPgetStationNum());
  portENTER_CRITICAL(&guard); memcpy(netSummary,text,sizeof(netSummary)); portEXIT_CRITICAL(&guard);
}
// Nonblocking socket writes: one slow browser or GCS cannot stall network service.
static int sendSome(NetworkClient &client, const uint8_t *bytes, size_t size) {
  const int n = ::send(client.fd(), bytes, size, MSG_DONTWAIT);
  if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) return 0;
  return n;
}
static void serviceHttp() {
  if (!httpClient.connected()) {
    httpClient.stop(); httpRequest = ""; httpResponse = ""; httpOffset = 0;
    httpClient = httpServer.accept();
    if (!httpClient) return;
    httpStart = millis(); httpClient.setNoDelay(true);
  }
  if (uint32_t(millis()-httpStart) > 3000) { httpClient.stop(); return; }
  if (httpResponse.isEmpty()) {
    unsigned budget = 512;
    while (budget-- && httpClient.available()) {
      httpRequest += char(httpClient.read());
      if (httpRequest.length() > 1024) { httpClient.stop(); return; }
      if (httpRequest.endsWith("\r\n\r\n")) {
        const bool dashboard = httpRequest.startsWith("GET / HTTP/");
        const bool status = httpRequest.startsWith("GET /api/status HTTP/");
        const bool lidarOnly = httpRequest.startsWith("GET /api/lidar HTTP/");
        const bool fcOnly = httpRequest.startsWith("GET /api/fc HTTP/");
        String body;
        if (dashboard) body = DASHBOARD;
        else if (status) body = statusJson();
        else if (lidarOnly) { xQueuePeek(snapshots, &device, 0); body = lidarJson(millis()); }
        else if (fcOnly) { xQueuePeek(snapshots, &device, 0); body = fcJson(millis()); }
        else body = "{\"error\":\"not found\"}";
        httpResponse = (dashboard || status || lidarOnly || fcOnly) ? "HTTP/1.1 200 OK\r\n" : "HTTP/1.1 404 Not Found\r\n";
        httpResponse += "Content-Type: "; httpResponse += dashboard ? "text/html; charset=utf-8" : "application/json";
        httpResponse += "\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: " + String(body.length()) + "\r\n\r\n" + body;
        break;
      }
    }
  }
  if (!httpResponse.isEmpty()) {
    const int n = sendSome(httpClient, reinterpret_cast<const uint8_t *>(httpResponse.c_str())+httpOffset,
                          httpResponse.length()-httpOffset);
    if (n < 0) { httpClient.stop(); return; }
    httpOffset += n;
    if (httpOffset == httpResponse.length()) httpClient.stop();
  }
}
static void task(void *) {
  WiFi.persistent(false); WiFi.setHostname("lidar-bridge");
  for (;;) { serviceWifi(); if (serversStarted) serviceHttp(); vTaskDelay(pdMS_TO_TICKS(2)); }
}
bool begin() {
  snapshots=xQueueCreate(1,sizeof(DeviceStatus));
  return snapshots && xTaskCreate(task,"lidar-network",12288,nullptr,1,nullptr)==pdPASS;
}
void publish(const DeviceStatus &s) { if (snapshots) xQueueOverwrite(snapshots,&s); }
void summary(char *out,unsigned size) {
  portENTER_CRITICAL(&guard); snprintf(out,size,"%s",netSummary); portEXIT_CRITICAL(&guard);
}
}
