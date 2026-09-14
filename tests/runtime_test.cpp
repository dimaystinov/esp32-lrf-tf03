#include "../firmware/lrf_hf_bridge/lrf_hf_bridge.ino"
#include <cassert>
namespace NetworkBridge {
bool begin(){return true;} void publish(const DeviceStatus &){} void summary(char *p,unsigned n){snprintf(p,n,"OFFLINE");}
}
void run(unsigned ms){auto start=millis();while(uint32_t(millis()-start)<ms)loop();}
int main(){
 setup();assert(autopilot.baud==115200);run(3500);
 assert(!Config::SIMULATE_DISTANCE && sent>=174 && !locked);
 assert(status.lidarHz==0 && status.txHz>=49 && status.txHz<=51);
 const std::vector<uint8_t> zero={0x59,0x59,0,0,0,0,0,0,0xB2};
 for(const auto &p:autopilot.tx)assert(p==zero);
 autopilot.tx.clear();
 auto inject=[](uint16_t cm){lidar.rx={0x5C,uint8_t(cm),uint8_t(cm>>8),uint8_t(~(uint8_t(cm)+uint8_t(cm>>8)))};loop();};
 selectBaud(0);
 for(int i=0;i<5;++i)inject(123);
 autopilot.tx.clear();
 run(20);assert(locked&&sent>0);
 const std::vector<uint8_t> expected={0x59,0x59,0x7B,0,0xF4,1,0,0,0x22};
 for(const auto &p:autopilot.tx)assert(p==expected);
 auto before=sent;
 for(int i=0;i<1000;++i)inject(123);
 assert(sent-before>=49&&sent-before<=51);
 autopilot.room=0;run(100);assert(dropped>0);before=sent;
 autopilot.room=512;inject(123);run(30);assert(sent>before);
 run(300);before=sent;run(100);assert(sent>=before+5);assert(autopilot.tx.back()==zero);
 for(uint16_t cm : {uint16_t(5001),uint16_t(10000),uint16_t(15000),uint16_t(15001),uint16_t(50000)}) {
  inject(cm);run(20);
  const auto &frame=autopilot.tx.back();
  assert(frame[2]==uint8_t(cm) && frame[3]==uint8_t(cm>>8));
  unsigned checksum=0;for(unsigned i=0;i<8;++i)checksum+=frame[i];
  assert(frame[8]==uint8_t(checksum));
 }
 inject(50001);run(20);assert(autopilot.tx.back()==zero);
 inject(65535);run(20);assert(autopilot.tx.back()==zero);
 inject(4);run(30);assert(autopilot.tx.back()==zero);
 inject(123);run(20);assert(autopilot.tx.back()==expected);
 lidar.rx={0x5C,0x7B,0,0};loop();run(20);assert(autopilot.tx.back()==zero);
 for(int i=0;i<5;++i)inject(123);
 run(20);assert(autopilot.tx.back()==expected);
 lidar.rx={0x5C};loop();run(45);assert(autopilot.tx.back()==zero);
 run(2100);assert(!locked);
 for(int i=0;i<5;++i)inject(234);
 run(20);assert(locked&&sent>before);
 autopilot.rx={1,2,3};loop();assert(status.fcRxBytes==3);
 for(int i=0;i<2200;++i)inject(234);
 assert(status.lidarHz>990 && status.lidarHz<1010 && status.txHz>=49 && status.txHz<=51);
 autopilot.room=0;run(2200);assert(status.lidarHz==0 && status.txHz==0);
 autopilot.room=512;run(2200);assert(status.txHz>=49 && status.txHz<=51);
 LrfParser check;
 for(unsigned cm=0;cm<65536;++cm){uint16_t value=0;check.reset();check.feed(0x5C,value);check.feed(cm&255,value);check.feed(cm>>8,value);assert(check.feed(uint8_t(~((cm&255)+(cm>>8))),value)==ParseResult::Good);assert(value==cm);}
 Measurement m;m.accept(100,0xFFFFFFF0,5,5000);assert(m.fresh(4,250));assert(!m.fresh(300,250));
 puts("PASS: TF03 exact frames 50Hz, no USB/lidar/network dependency, backpressure, RX diagnostic, exhaustive lidar decoding, stale rollover");
}
