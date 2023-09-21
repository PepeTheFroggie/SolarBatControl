
// 4CH Relay  16 14 12 13
#define LED1    5 
#define relay1 16 
#define relay2 14 
#define relay3 12 
#define relay4 13 

// 2CH
// #define LED1   16 
// #define relay1  4 
// #define relay2  5 


void switchinit()
{
  pinMode(relay1, OUTPUT); digitalWrite(relay1,LOW);
  pinMode(relay2, OUTPUT); digitalWrite(relay2,LOW);
  pinMode(relay3, OUTPUT); digitalWrite(relay3,LOW);
  pinMode(relay4, OUTPUT); digitalWrite(relay4,LOW);
}

#define wron_waitcycles 5
int wronct;

// 2 and 4 in parallel WR
// 3 preload WR cap
// 1 charger

void wr_on(bool wron)
{
  if (wron != WRstate) // state change
  {
    if (wron)  // switch ON
    {
      if (wronct>= wron_waitcycles)  //delay switch on by 2 cycles
      {
        digitalWrite(relay3,HIGH);
        delay(400);
        digitalWrite(relay4,HIGH);
        digitalWrite(relay2,HIGH);
        delay(100); 
        digitalWrite(relay3,LOW);
        WRstate = true;
      } 
      else wronct++;
    }
    else  // switch OFF
    {
      digitalWrite(relay4,LOW);
      digitalWrite(relay2,LOW);
      digitalWrite(relay3,LOW); 
      WRstate = false;  
      wronct = 0;   
    }
  }
}

void chg_on(bool chgon)
{
  CHGstate = chgon;
  digitalWrite(relay1,chgon);
}
