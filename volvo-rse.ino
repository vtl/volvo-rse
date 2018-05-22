/*
   VOLVO Rear Seat Entertainment display controller

   Pin 2 (pullup high): GND - on, +3.3 - off

   Tested with headrest display from 2007 XC70.
   Codes snooped from 2006-08 XC90 DVD player

   (c) 2018 Vitaly Mayatskikh <vitaly@gravicappa.info>
*/

#include <PrintEx.h>
#include <due_can.h>

StreamEx serial = Serial;//USB;

#define DISPLAY_A_ON 3
#define DISPLAY_B_ON 2

void print_frame(const char *s, CAN_FRAME *in)
{
  serial.printf("CAN_FRAME for %s ID 0x%lx: ", s, in->id);
  for (int i = 0; i < 8; i++)
    serial.printf("0x%02x ", in->data.byte[i]);
  serial.printf("\n");
}

void can_callback(CAN_FRAME *in)
{
  print_frame("in", in);
}

void setup_canbus()
{
  serial.println("setup CAN-bus...\n");
  Can0.begin(CAN_BPS_500K);
  Can0.setRXFilter(0, 0, true);
  Can0.setRXFilter(3, 0, false);
  Can0.setGeneralCallback(can_callback);
  Can1.begin(CAN_BPS_500K);
  Can1.setRXFilter(0, 0, true);
  Can1.setRXFilter(3, 0, false);
  Can1.setGeneralCallback(can_callback);

}

void setup()
{
  Serial.begin(115200);
  serial.println("start");
  setup_canbus();
  pinMode(DISPLAY_A_ON, INPUT);//_PULLUP);
  pinMode(DISPLAY_B_ON, INPUT);//_PULLUP);
  serial.println("done"); 
  delay(5000);
}

void send_frame(const char *name, CANRaw *can, unsigned char *data)
{
  CAN_FRAME out;

  memset(out.data.bytes, 0, 8);
  out.id = 0x0601;
  out.extended = false;
  out.priority = 4;
  out.length = 8;
  memcpy(out.data.byte, data, 8);
  print_frame(name, &out);
  can->sendFrame(out);
}

unsigned char on[5][9]  = {{ 0x03, 0x90, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 10 }, //40
                           { 0x03, 0xA3, 0x08, 0x01, 0x01 /* 1=A,2=B */, 0x00, 0x00, 0x00, 1 }, // 100
                           { 0x01, 0xb5, 0x07, 0x01, 0x01, 0x00, 0x00, 0x00, 1 }, //40
                           { 0x03, 0xB8, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 1},
                           { 0x03, 0x90, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 1 }};

unsigned char off[2][9] = {{ 0x01, 0xb5, 0x0a, 0x01, 0x01, 0x00, 0x00, 0x00, 1 },
                           { 0x03, 0x90, 0x0b, 0x01, 0x00, 0x00, 0x00, 0x00, 0 }};

#define A 1 // true
#define B 0 // false

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
    send_frame(a ? "A" : "B", a ? &Can1 : &Can0, p);
    delay((long)p[8] * 100);
  }
}

void loop()
{
  static bool last_display_status[2] = { false, false };
  bool display_status[2];
  
  display_status[A]= !digitalRead(DISPLAY_A_ON);
  display_status[B]= !digitalRead(DISPLAY_B_ON);

//  display_status[A]= digitalRead(DISPLAY_A_ON);
//  display_status[B]= digitalRead(DISPLAY_B_ON);

//  serial.printf("A last_display_status %d, display_status %d\n", last_display_status[A], display_status[A]);
//  serial.printf("B last_display_status %d, display_status %d\n", last_display_status[B], display_status[B]);

  for (int i = 0; i < 2; i++) {
    if (last_display_status[i] != display_status[i]) {
      last_display_status[i] = display_status[i];
      do_display(i, display_status[i]);
    }
  }
  delay(100);
}

