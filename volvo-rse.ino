/*
   VOLVO Rear Seat Entertainment display controller

   Pin 2 (pullup high): GND - on, +3.3 - off

   Tested with headrest display from 2007 XC70.
   Codes snooped from 2006-08 XC90 DVD player

   (c) 2018 Vitaly Mayatskikh <vitaly@gravicappa.info>
*/

#include <PrintEx.h>
#include <due_can.h>

StreamEx serial = SerialUSB;

#define DISPLAY_ON 2

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
  serial.println("start");
  setup_canbus();
  pinMode(DISPLAY_ON, INPUT_PULLUP);
  serial.println("done"); 
}

void send_frame(CANRaw *can, unsigned char *data)
{
  CAN_FRAME out;

  memset(out.data.bytes, 0, 8);
  out.id = 0x0601;
  out.extended = false;
  out.priority = 4;
  out.length = 8;
  memcpy(out.data.byte, data, 8);
  print_frame("out", &out);
  can->sendFrame(out);
}

unsigned char on[2][8]  = {{ 0x03, 0x90, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00 },
                           { 0x01, 0xb5, 0x07, 0x01, 0x01, 0x00, 0x00, 0x00 }};

unsigned char off[2][8] = {{ 0x01, 0xb5, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00 },
                           { 0x03, 0x90, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00 }};

void do_display(bool status)
{
  static int seq_id = 0;
  int delay_ms;
  unsigned char *p1, *p2;
  
  if (status) {
    p1 = on[0];
    p2 = on[1];
    delay_ms = 333;  
  } else {
    p1 = off[0];
    p2 = off[1];
    delay_ms = 100;
  }

  p1[2] = seq_id++;
  p2[2] = seq_id++;

  send_frame(&Can0, p1);
  send_frame(&Can1, p1);
  delay(delay_ms);
  send_frame(&Can0, p2);
  send_frame(&Can1, p2);  
}

void loop()
{
  static bool last_display_status = false;
  bool display_status = !digitalRead(DISPLAY_ON);

//  serial.printf("last_display_status %d, display_status %d\n", last_display_status, display_status);

  if (last_display_status != display_status) {
    last_display_status = display_status;
    do_display(display_status);
  }
  delay(1000);
}

