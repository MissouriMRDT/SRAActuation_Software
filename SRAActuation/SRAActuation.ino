#include "RoveComm.h"
#include "RoveStmVnhPwm.h"
#include "SRAActuation.h"

void setup()
{
  Serial.begin(115200);
  delay(10);

  RoveComm.begin(RC_SRAACTUATIONBOARD_FOURTHOCTET);
  delay(10);

  pinMode(SPECTZ_PB, INPUT);
  pinMode(SPECTX_PB, INPUT);
  pinMode(SPECTY_PB, INPUT);
  pinMode(DIR_SW, INPUT);

  pinMode(SPECTZ_UPP_LS, INPUT);
  pinMode(SPECTZ_MID_LS, INPUT);
  pinMode(SPECTZ_LOW_LS, INPUT);
  pinMode(SPECTX_CENTER, INPUT);

  delay(10);
  Spectz.attach(SPECTZ_HEADER);
  Spectx.attach(SPECTX_HEADER);
  Specty.attach(SPECTY_HEADER);

  
}

void loop()
{
  readRoveComm();
  checkButtons();
  checkLS();
  moveToPos();
  adjustX();
  writeSpeeds();
  sendStates();
}

void readRoveComm()
{
  rovecomm_packet packet = RoveComm.read();

  switch(packet.data_id)
  {
    case RC_SRAACTUATION_VERTICALOPENLOOP_DATAID:
    {
      if(packet.data[0] != 0) do_to_pos_z = false;
      z_speed = packet.data[0];
      break;
    }
    case RC_SRAACTUATION_VERTICALTOPOSITION_DATAID:
    {
      do_to_pos_z = true;
      target_position = packet.data[0];
      break;
    }
    case RC_SRAACTUATION_SPECTROMETERMOVE_DATAID:
    {
      x_speed = packet.data[0];
      y_speed = packet.data[1];
      break;
    }
  }
}

void checkButtons()
{
  if(digitalRead(SPECTZ_PB))
  {
    z_speed =  BUTTON_SPEED * (digitalRead(DIR_SW)? 1 : -1);
    in_to_pos = false;
  }
  if(digitalRead(SPECTX_PB))
  {
    x_speed =  BUTTON_SPEED * (digitalRead(DIR_SW)? 1 : -1);
  }
  if(digitalRead(SPECTY_PB))
  {
    y_speed =  BUTTON_SPEED * (digitalRead(DIR_SW)? 1 : -1);
  }
}

void checkLS()
{
  /*if(!digitalRead(SPECTZ_LOW_LS))
  {
    ls_pressed = 0;
    if(z_speed<0) z_speed = 0;
    current_position = 0;
  }
  else */if(!digitalRead(SPECTZ_MID_LS))
  {
    ls_pressed = 1;
    current_position = 1;
  }
  else if(!digitalRead(SPECTZ_UPP_LS))
  {
    ls_pressed = 2;
    if(z_speed>0) z_speed = 0;
    current_position = 2;
  }
  else //No LS pressed
  {
    if(ls_pressed == 1)
    {
     ls_pressed = 4;
     if(z_speed <= 0) current_position = 0;
     else if (z_speed > 0) current_position = 1;
    }
    else if(ls_pressed == 2)
    {
      ls_pressed = 4;
      current_position = 1;
    }
  }
  if(x_centered && digitalRead(SPECTX_CENTER))
  {
    x_pos = x_speed < 0? -1:1;
  }
  x_centered = !digitalRead(SPECTX_CENTER);

  //Serial.println(current_position);
}

void moveToPos()
{
  Serial.println(do_to_pos_z);
  if(do_to_pos_z)
  {
    z_speed = 0;
    if((target_position == 2) && digitalRead(SPECTZ_UPP_LS))
    {
      z_speed = BUTTON_SPEED;
    }
    else if((target_position == 1) && digitalRead(SPECTZ_UPP_LS))
    {
      z_speed = 500* ((target_position - current_position > 0)?1:-1);
    }
  }
}

void writeSpeeds()
{
  //Serial.println("Speed"); Serial.println(z_speed); Serial.println(x_speed); Serial.println(y_speed);
  
  Spectz.drive(z_speed);
  Specty.drive(y_speed);
  Spectx.drive(x_speed);
}

void sendStates()
{
  static int last_send = 0;
  if(millis()-last_send > 500)
  {
    last_send = millis();
    RoveComm.write(RC_SRAACTUATION_VERTICALPOSITION_HEADER, current_position);
  }
}

void adjustX()
{
  if(current_position >= 1)
  {
    x_speed = 0;
    if(!x_centered)
    {
      x_speed = -x_pos * 500;
    }
  }
}