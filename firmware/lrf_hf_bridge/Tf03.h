#pragma once
#include <stdint.h>
// TF03 UART: centimetres, signal strength, two reserved zero bytes, sum of bytes 0..7.
inline void encodeTf03(uint8_t *p, uint16_t cm) {
  p[0]=0x59; p[1]=0x59; p[2]=uint8_t(cm); p[3]=uint8_t(cm>>8);
  p[4]=cm ? 0xF4 : 0; p[5]=cm ? 0x01 : 0; // nominal strength 500; zero for invalid distance
  p[6]=0; p[7]=0; p[8]=0;
  for (unsigned i=0;i<8;++i) p[8]=uint8_t(p[8]+p[i]);
}
