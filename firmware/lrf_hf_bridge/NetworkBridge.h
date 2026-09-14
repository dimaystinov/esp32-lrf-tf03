#pragma once
#include <stdint.h>
struct DeviceStatus {
  uint32_t uptime=0,lidarBaud=0,sampleTime=0;
  uint16_t cm=0;
  float lidarHz=0;
  bool locked=false,fresh=false;
  uint32_t good=0,bad=0,rejected=0,sent=0,txDrop=0,fcRxBytes=0;
};
namespace NetworkBridge {
bool begin();
void publish(const DeviceStatus &status);
void summary(char *out,unsigned size);
}
