/*   STM32F103C8 Blue Pill ( PC13)

  Serialport set and display RTC clock , Write by CSNOL https://github.com/csnol/STM32-Examples
  based on https://github.com/rogerclarkmelbourne/Arduino_STM32
  get Unix epoch time from https://www.epochconverter.com/ ;
*/

#include <SPI.h>
#include <RF22.h>
#include <RTClock.h>
#include <EEPROM.h>

#define MORSETAB 39


#define pinSND PA1
#define pinSDN PA0


RF22 rf22(PA4, PA2, &Hardware_spi);

typedef struct{
    uint8_t sb=0xAA;
    uint8_t csgn[10]="UR5TLZ";
    uint8_t qth[10]="KN39MJ";
    uint16_t csgnprd=900;
    uint16_t btail=3;
    uint8_t tzone=2;
    bool beep=true;
    bool csbkn=true; 
    float freq=433.52;
}beacon;
beacon beacon_r ;

uint16 DataWrite = 0xAA;
uint16 AddressWrite = 0x10;

uint8_t temp[8];
uint16 Status=0;
uint16 Data;

uint8_t rssiprev=0;

RTClock rtclock (RTCSEL_LSE,0x7fff); // 0x7fff
int timezone = 2;      // change to your timezone
time_t tt, tt1;
tm_t mtt;
uint8_t dateread[20];
bool dispflag = true;

uint8_t ts= beacon_r.btail+2;
bool alarmflag = false;
bool bipflag = false;
bool callorQTH = true;
uint8_t countSQL=0;
//-----------------------------------------------------------------------------
                           //B3456789
const char  sevenseg[] =  {0b00111111,/*0*/
                           0b00001001,/*1*/
                           0b01011110,/*2*/
                           0b01011011,/*3*/
                           0b01101001,/*4*/
                           0b01110011,/*5*/
                           0b01110111,/*6*/
                           0b00011001,/*7*/
                           0b01111111,/*8*/
                           0b01111011};/*9*/

//-----------------------------------------------------------------------------
int morse[2][MORSETAB] = {
    {'A','B','C','D','E','F',
     'G','H','I','J','K','L',
     'M','N','O','P','Q','R',
     'S','T','U','V','W','X',
     'Y','Z','0','1','2','3',
     '4','5','6','7','8','9',
     ' '},
    {B00000110,B00010111,B00010101,B00001011,B00000011,B00011101,
     B00001001,B00011111,B00000111,B00011000,B00001010,B00011011,
     B00000100,B00000101,B00001000,B00011001,B00010010,B00001101,
     B00001111,B00000010,B00001110,B00011110,B00001100,B00010110,
     B00010100,B00010011,B00100000,B00110000,B00111000,B00111100,
     B00111110,B00111111,B00101111,B00100111,B00100011,B00100001,
     B00101110}
};

int wpm=12;
int frq=1000;
int del=3000;
int bip=500;
int ditpause=1200/wpm;
int dashpause=ditpause*3;
int elementpause=ditpause;
int charpause=ditpause*3;
int wordpause=ditpause*7;

//-----------------------------------------------------------------------------
const char * weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const char * months[] = {"Dummy", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
//-----------------------------------------------------------------------------

uint8_t str2month(const char * d)
{
    uint8_t i = 13;
    while ( (--i) && strcmp(months[i], d)!=0 );
    return i;
}
//-----------------------------------------------------------------------------
const char * delim = " :";
char s[128]; // for sprintf
//-----------------------------------------------------------------------------
void ParseBuildTimestamp(tm_t & mt)
{
    // Timestamp format: "Dec  8 2017, 22:57:54"
    sprintf(s, "Timestamp: %s, %s\n", __DATE__, __TIME__);
    //Serial.print(s);
    char * token = strtok(s, delim); // get first token
    // walk through tokens
    while( token != NULL ) {
        uint8_t m = str2month((const char*)token);
        if ( m>0 ) {
            mt.month = m;
            //Serial.print(" month: "); Serial.println(mt.month);
            token = strtok(NULL, delim); // get next token
            mt.day = atoi(token);
            //Serial.print(" day: "); Serial.println(mt.day);
            token = strtok(NULL, delim); // get next token
            mt.year = atoi(token) - 1970;
            //Serial.print(" year: "); Serial.println(mt.year);
            token = strtok(NULL, delim); // get next token
            mt.hour = atoi(token);
            //Serial.print(" hour: "); Serial.println(mt.hour);
            token = strtok(NULL, delim); // get next token
            mt.minute = atoi(token);
            //Serial.print(" minute: "); Serial.println(mt.minute);
            token = strtok(NULL, delim); // get next token
            mt.second = atoi(token);
            //Serial.print(" second: "); Serial.println(mt.second);
        }
        token = strtok(NULL, delim);
    }
}

void alarmset(void)
{
  rtclock.detachAlarmInterrupt();
  rtclock.createAlarm(blink, (rtclock.getTime() + beacon_r.csgnprd)); 
}

//-----------------------------------------------------------------------------
// This function is called in the attachSecondsInterrupt
//-----------------------------------------------------------------------------
void SecondCount ()
{
  tt++;
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  if ((ts>=0)&&(ts<= beacon_r.btail))
  {
    if (ts==0)
    {
     bipflag = true;
    }
    ts--;
    //digitalWrite(pinPTT, HIGH);
  }
  else
  {
    //digitalWrite(pinPTT, LOW);
  }
}
//-----------------------------------------------------------------------------
// This function is called in the attachAlarmInterrupt
//-----------------------------------------------------------------------------
void blink ()
{
  //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  alarmflag = true;
  //Serial.println(" Blink ");
}

void setup()
{

  Serial.begin(115200);
  
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(pinSND, OUTPUT);
  digitalWrite(pinSND, LOW);
  
  pinMode(pinSDN, OUTPUT);
  digitalWrite(pinSDN, LOW);

  //while (!Serial); delay(1000);
  //ParseBuildTimestamp(mtt);  // get the Unix epoch Time counted from 00:00:00 1 Jan 1970
  //tt = rtclock.makeTime(mtt) + 25; // additional seconds to compensate build and upload delay
  //rtclock.setTime(tt);


  while (!rf22.init())
    {
      Serial.println("RF22 init failed");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        digitalWrite(pinSDN, HIGH);
        delay(500);
        digitalWrite(pinSDN, LOW); 
    }

  rf22.setFrequency(beacon_r.freq, 0.05);
  rf22.setTxPower(RF22_TXPOW_20DBM);
  rf22.setModeRx(); 
  rf22.spiWrite(RF22_REG_71_MODULATION_CONTROL2, 0x42);
  rf22.spiWrite(RF22_REG_0B_GPIO_CONFIGURATION0, 0x12);
  
  tt1 = tt;
  rtclock.attachAlarmInterrupt(blink);// Call blink
  rtclock.attachSecondsInterrupt(SecondCount);// Call SecondCount

  alarmset();   
}


char playmorse(char* morsearray)
{

  char arindx=0;
  rf22.setModeTx();
  while (arindx!=strlen(morsearray))
  {
       char i=0;
       while (i<MORSETAB)
        {
          
          if (*(morsearray+arindx)==' ')
          {
            delay(wordpause);
            arindx++;
            i=0;
          }
          else if (*(morsearray+arindx)==morse[0][i])
          {

            unsigned char charindx=B10000000;
            while ((morse[1][i]&charindx)==0)
            {
              charindx=charindx>>1;
            }
            charindx=charindx>>1;
            while (charindx!=0)
            {
              //Serial.write(morse[1][i]&&charindx);
              //Serial.println(charindx,DEC);
              
              if ((morse[1][i]&charindx)>0)
              {
                 //Serial.print(" DIT");
                 digitalWrite(LED_BUILTIN, LOW);
                 tone(pinSND, frq);   
                 delay(ditpause);                  
                 digitalWrite(LED_BUILTIN, HIGH);
                 noTone(pinSND);
                 delay(elementpause);
              }
              else
              {
                 //Serial.print(" DASH");
                 digitalWrite(LED_BUILTIN, LOW);
                 tone(pinSND, frq);   
                 delay(dashpause);                  
                 digitalWrite(LED_BUILTIN, HIGH);
                 noTone(pinSND);
                 delay(elementpause);
              }
              
              charindx=charindx>>1;
            }
          }
          i++;
        }
    arindx++;
    delay(charpause-elementpause);
  }
  delay(del); 
  rf22.setModeRx(); 
}

void str(void)
{
      Serial.println("Reading settings:");
      unsigned char *myPtr = (unsigned char *)&beacon_r;
      const unsigned char *byteToRead;
      int numberOfBytes = sizeof(beacon);
      uint16 AdrRMem=0;
      for(byteToRead=myPtr; numberOfBytes--; ++byteToRead)  
      { 
       Status = EEPROM.read(AddressWrite+AdrRMem, &Data);
       *myPtr++=Data;
       Serial.print(Data);
       AdrRMem++;
       Serial.print(" ");
      }
}

//-----------------------------------------------------------------------------
void loop()
{
  if ( Serial.available()) {
    uint8_t b=Serial.available();
    //Serial.println(b);
    for (uint8_t i = 0; i<b; i++) {
      dateread[i] = Serial.read();
      //Serial.print(dateread[i],HEX);
      //Serial.print(" ");
    }
    //Serial.println(" ");
    if (dateread[b]!=0x0A)
    {
      dateread[b]=0x0A;
      //Serial.println("0x0A");
    }
    Serial.flush();
    if (strstr((char*)dateread,"eph"))
    {
      tt = atol((char*)dateread+3);
      rtclock.setTime(rtclock.TimeZone(tt, beacon_r.tzone)); //adjust to your local date
      Serial.println("Time set:");
      alarmset();
    }
    
    if (strstr((char*)dateread,"frq"))
    {
      beacon_r.freq = atof((char*)dateread+3);
      rf22.setFrequency(beacon_r.freq, 0.05);
      Serial.println("Frequency set:");
      Serial.println(beacon_r.freq);
    }    
    
    if (strstr((char*)dateread,"csn"))
    {
      Serial.println("Callsign set:");
      strncpy((char*)beacon_r.csgn, (char*)dateread+4, strlen((char*)dateread)-5);
      Serial.println((char*)beacon_r.csgn);
    } 
    
    if (strstr((char*)dateread,"qth"))
    {
      Serial.println("QTH set:");
      strncpy((char*)beacon_r.qth, (char*)dateread+4, strlen((char*)dateread)-5);
      Serial.println((char*)beacon_r.qth);
    }  

    if (strstr((char*)dateread,"csp"))
    {
      Serial.println("Callsign beakon period set:");
      strncpy((char*)temp, (char*)dateread+4, strlen((char*)dateread)-5);
      beacon_r.csgnprd=atoi((char*)temp);
      Serial.println(beacon_r.csgnprd); 
      alarmset();    
    } 

    if (strstr((char*)dateread,"btl"))
    {
      Serial.println("Beacon tail period set:");
      strncpy((char*)temp, (char*)dateread+4, strlen((char*)dateread)-5);
      beacon_r.btail=atoi((char*)temp);
      Serial.println(beacon_r.btail);     
    } 

    if (strstr((char*)dateread,"tzn"))
    {
      Serial.println("Time zone set");
      strncpy((char*)temp, (char*)dateread+4, strlen((char*)dateread)-5);
      beacon_r.tzone=atoi((char*)temp);
      Serial.println(beacon_r.tzone);     
    }
     
    if (strstr((char*)dateread,"bip"))
    {
      Serial.println("Rodger beep  set:");
      if (dateread[4]=='1')
      {
        Serial.println("True");
        beacon_r.beep=true;
      }
      else
      {
        Serial.println("False");
        beacon_r.beep=false;
      }
    }
        
    if (strstr((char*)dateread,"bcn"))
    {
      Serial.println("Callsign beacon set:");
      if (dateread[4]=='1')
      {
        Serial.println("True");
        beacon_r.csbkn=true;
      }
      else
      {
        Serial.println("False");
        beacon_r.csbkn=false;
      }
    }
         
    if (strstr((char*)dateread,"st?"))
    {
      Serial.println("Settings list:");
      Serial.print("Callsign:"); Serial.println((char*)beacon_r.csgn);
      Serial.print("QTH:"); Serial.println((char*)beacon_r.qth);
      Serial.print("Beacon period:"); Serial.println(beacon_r.csgnprd);
      Serial.print("Beacon tail:"); Serial.println(beacon_r.btail);
      Serial.print("Time zone:"); Serial.println(beacon_r.tzone);
      Serial.print("Rodger beep enabled:"); Serial.println(beacon_r.beep);
      Serial.print("Beacon enabled:"); Serial.println(beacon_r.csbkn); 
      Serial.print("Frequency set:"); Serial.println(beacon_r.freq);   
    }
          
    if (strstr((char*)dateread,"stw"))
    {    
      Serial.println("Saving settings:");
      unsigned char *myPtr = (unsigned char *)&beacon_r;
      const unsigned char *byteToSend;
      int numberOfBytes = sizeof(beacon);
      uint16 AdrWMem=0;
      for(byteToSend=myPtr; numberOfBytes--; ++byteToSend)  
      { 
       DataWrite=*byteToSend;
       Serial.print(DataWrite);
       Status = EEPROM.write(AddressWrite+AdrWMem, DataWrite);
       AdrWMem++;
       Serial.print(" ");
      }
    }

    if (strstr((char*)dateread,"str"))
    {    
      str();
    }
  }

  
  if (bipflag == true)
  {
     bipflag = false;
     //digitalWrite(pinPTT, HIGH);
     if (beacon_r.beep == true)
     {
      delay(2*bip);
      tone(pinSND, frq);   
      delay(bip);                  
      noTone(pinSND);
     }
  }

  if(alarmflag == true)
  {
    alarmflag = false;
    alarmset();
    if (beacon_r.csbkn == true)
    {  
      Serial.println(" Alarm ");
      delay(bip);
      if (callorQTH==true)
      {
        playmorse("UR5TLZ");
        Serial.println("CallSign");
      }
      else
      {
        playmorse("KN39MJ");
        Serial.println("QTH");
      }
     callorQTH=!callorQTH;
     delay(bip);
    }
  }


  if((rf22.rssiRead()>rssiprev+20)||(rf22.rssiRead()<rssiprev-20))
  {
   delay(wordpause*3);
   rf22.setModeTx();
   delay(wordpause*2);
   rf22.setTxPower(RF22_TXPOW_1DBM);
   tone(pinSND,700,1000); 
   delay(500);
   noTone(pinSND);
   delay(wordpause);
   rf22.setTxPower(RF22_TXPOW_11DBM);
   tone(pinSND,800,1000); 
   delay(500);
   noTone(pinSND);
   delay(wordpause);
   rf22.setTxPower(RF22_TXPOW_20DBM);
   tone(pinSND,900,1000); 
   delay(500);
   noTone(pinSND);
   delay(wordpause);
   playmorse("ECHO");
   delay(wordpause);
   rf22.setModeRx();
 }
 Serial.print ("RSSI: ");
 Serial.println (rf22.rssiRead());
 rssiprev=rf22.rssiRead();
 delay(ditpause); 

  
  if (tt1 != tt && dispflag == true )
  {
    tt1 = tt;
    // get and print actual RTC timestamp
    rtclock.breakTime(rtclock.now(), mtt);
    sprintf(s, "RTC timestamp: %s %u %u, %s, %02u:%02u:%02u\n",
      months[mtt.month], mtt.day, mtt.year+1970, weekdays[mtt.weekday], mtt.hour, mtt.minute, mtt.second);
    Serial.print(s);
    if (mtt.second==0)
    {
      //--------------------------------------------------------------------------------------------------------
    }
  }
}
