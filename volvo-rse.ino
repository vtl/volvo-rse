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
  Serial.begin(115200);
  serial.println("start");
  setup_canbus();
  pinMode(DISPLAY_ON, INPUT_PULLUP);
  serial.println("done"); 
}

void send_frame(char *name, CANRaw *can, unsigned char *data)
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

unsigned char on_a[4][9]  = {{ 0x03, 0x90, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 4 },
                             { 0x01, 0xb5, 0x07, 0x01, 0x01, 0x00, 0x00, 0x00, 160 },
                             { 0x03, 0xA3, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 100 },
                             { 0x03, 0xB8, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0}};

unsigned char on_b[4][9]  = {{ 0x03, 0x90, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 4 },
                             { 0x01, 0xb5, 0x07, 0x01, 0x01, 0x00, 0x00, 0x00, 160 },
                             { 0x03, 0xA3, 0x08, 0x01, 0x02, 0x00, 0x00, 0x00, 100 },
                             { 0x03, 0xB8, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0}};

unsigned char off[2][9] = {{ 0x01, 0xb5, 0x0a, 0x01, 0x01, 0x00, 0x00, 0x00, 1 },
                           { 0x03, 0x90, 0x0b, 0x01, 0x00, 0x00, 0x00, 0x00, 0 }};

void do_display(bool status)
{
  static int seq_id = 1;
  unsigned char *pa[8], *pb[8];
  int n;

  if (status) {
    n = 4;
    for (int i = 0; i < n; i++) {
      pa[i] = on_a[i];
      pb[i] = on_b[i];
    }
  } else {
    n = 2;
    for (int i = 0; i < n; i++) {
      pa[i] = off[i];
      pb[i] = off[i];
    }
  }

  for (int i = 0; i < n; i++) {
    pa[i][2] = seq_id;
    pb[i][2] = seq_id;
    seq_id++;
    send_frame("A", &Can0, pa[i]);
    send_frame("B", &Can1, pb[i]);
    delay(pa[i][8] * 100);
  }
}

void loop()
{
  static bool last_display_status = false;
  bool display_status = !digitalRead(DISPLAY_ON);

  serial.printf("last_display_status %d, display_status %d\n", last_display_status, display_status);

  if (last_display_status != display_status) {
    last_display_status = display_status;
    do_display(display_status);
  }
  delay(1000);
}

