/*
   VOLVO Rear Seat Entertainment display controller

   Pin 2 (pullup high): GND - on, +3.3 - off
   Pin 3 (pullup high): GND - on, +3.3 - off

   Tested with headrest display from 2007 XC70.
   Codes snooped from 2006-08 XC90 DVD player

   (c) 2018-2019 Vitaly Mayatskikh <vitaly@gravicappa.info>
*/

//#define HAS_B

#include <mcp_can.h>
#include <SPI.h>

MCP_CAN CAN0(10);
#ifdef HAS_B
MCP_CAN CAN1(11);
#endif

#define DISPLAY_A_ON 2
#define DISPLAY_B_ON 3

void setup()
{
  Serial.begin(115200);
  Serial.println("setup CAN-bus...");

  while (CAN0.begin(CAN_500KBPS) != CAN_OK) {
    delay(250);
  }
  Serial.println("A OK!");

#ifdef HAS_B
  while (CAN1.begin(CAN_500KBPS) != CAN_OK) {
    delay(250);
  }
  Serial.println("B OK!");
#endif
 
  pinMode(DISPLAY_A_ON, INPUT_PULLUP);
  pinMode(DISPLAY_B_ON, INPUT_PULLUP);
  delay(5000); // give some time for monitors to init
}

unsigned char on[5][9]  = {{ 0x03, 0x90, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 10 }, //40
                           { 0x03, 0xA3, 0x08, 0x01, 0x01 /* 1=A,2=B */, 0x00, 0x00, 0x00, 1 }, // 100
                           { 0x01, 0xb5, 0x07, 0x01, 0x01, 0x00, 0x00, 0x00, 1 }, //40
                           { 0x03, 0xB8, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 1},
                           { 0x03, 0x90, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 1 }};

unsigned char off[2][9] = {{ 0x01, 0xb5, 0x0a, 0x01, 0x01, 0x00, 0x00, 0x00, 1 },
                           { 0x03, 0x90, 0x0b, 0x01, 0x00, 0x00, 0x00, 0x00, 0 }};

#define A 1
#define B 0

void do_display(bool a, bool status)
{
  static int seq_id = 1;
  unsigned char *p;
  int n = status ? 5 : 2;

  for (int i = 0; i < n; i++) {
    p = status ? on[i] : off[i];
    p[2] = seq_id++;
    if (status && i == 1) {
      p[4] = a ? 1 : 2; /* IR channel A/B */
    }
    if (a)
      CAN0.sendMsgBuf(0x601, 0, 8, p);
#ifdef HAS_B
    else
      CAN1.sendMsgBuf(0x601, 0, 8, p);
#endif
    delay((long)p[8] * 100);
  }
}

void loop()
{
  static bool last_display_status[2] = { false, false };
  bool display_status[2];
  
  display_status[A]= !digitalRead(DISPLAY_A_ON);
  display_status[B]= !digitalRead(DISPLAY_B_ON);

  for (int i = 0; i < 2; i++) {
    if (last_display_status[i] != display_status[i]) {
      Serial.print("A last_display_status "); Serial.print(last_display_status[A]); Serial.print(" display_status "); Serial.println(display_status[A]);
      Serial.print("B last_display_status "); Serial.print(last_display_status[B]); Serial.print(" display_status "); Serial.println(display_status[B]);
      last_display_status[i] = display_status[i];
      do_display(i, display_status[i]);
    }
  }
  delay(100);
}
