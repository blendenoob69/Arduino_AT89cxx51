/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/AnalogReadSerial
*/
#define BIT0 (1<<0)
#define BIT1 (1<<1)
#define BIT2 (1<<2)
#define BIT3 (1<<3)
#define BIT4 (1<<4)
#define BIT5 (1<<5)
#define BIT6 (1<<6)
#define BIT7 (1<<7)

#define XON 0x11
#define XOFF 0x13

unsigned char AT89cXXADDRESS[2];  // two byte address buffer
unsigned char AT89cXXRAM[8];      // eight byte RAM buffer
int AT89cXXADDRESSpointer;
char AT89CXXchip=2;               //2 = 89x2051

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(19200);
  DDRD = DDRD & 0x03; //ARDUINO D0..1 Rx Tx
  DDRB = DDRB & 0xFD; //PB0..1 as MSB
  DDRB = DDRB | 0x3C; //PB2=PROG12V, PB3=RESET, PB4=PROG, PB5=ENABLE
  DDRC = DDRC & ~0x20; //PC5=BUSSY imput
  DDRC = DDRC | 0x10; //PC4=CLOCK output
  PORTC &= ~0x10;     //CLOCK stays LOW (active HIGH)
}

//----------------------------
char nibble2hex(char nibbleval)
{
  if (nibbleval < 10)
  {
    nibbleval += '0';
  }
  else nibbleval += 'A' - 10;
  return nibbleval;
}
unsigned char hex2nibble(char hexval)
{
  unsigned char nibbleval;
  if (hexval < 'a' && hexval < 'A')
  {
    nibbleval = hexval - '0';
  }
  else if (hexval >= 'a') nibbleval = hexval - 'a' + 10;
  else nibbleval = hexval - 'A' + 10;
  return nibbleval;
}

void SendBurnerMenue()
{
  Serial.print("\n? - Prints this help menue\n");
  Serial.print("R - Read DATA\n");
  Serial.print("W - Writes DATA\n");
  Serial.print("Z - DATA - test purpses\n");
  Serial.print("V - Reads SIGNATURE of the chip\n");
  Serial.print("Tn- n=1 * 89_1051 n=2 89_2051 * n=4 89_4051\n");
}


// Ddirection = 1 ==> DATAoutput
// Ddirection = 0 ==> DATAinput
void SetDataDirection(char Ddirection)
{
  if (Ddirection == 1)
  {
    DDRD = DDRD | ~0x03; //ARDUINO D0..1 Rx Tx
    DDRB = DDRB | 0x03; //PB0..1 as MSB
  }
  else
  {
    DDRD = DDRD & 0x03; //ARDUINO D0..1 Rx Tx
    DDRB = DDRB & ~0x03; //PB0..1 as MSB
  }
}

// OUTput pins are
// PORTD 2..7 LSB..MSB
// PORTB 0..1 HSB
// in total eight bits
char PutData2Port(unsigned char dataByte)
{
  (dataByte & 0x80) ? (PORTB = PORTB | 0x02) : (PORTB = PORTB & ~0x02);
  (dataByte & 0x40) ? (PORTB = PORTB | 0x01) : (PORTB = PORTB & ~0x01);
  (dataByte & 0x20) ? (PORTD = PORTD | 0x80) : (PORTD = PORTD & ~0x80);
  (dataByte & 0x10) ? (PORTD = PORTD | 0x40) : (PORTD = PORTD & ~0x40);
  (dataByte & 0x08) ? (PORTD = PORTD | 0x20) : (PORTD = PORTD & ~0x20);
  (dataByte & 0x04) ? (PORTD = PORTD | 0x10) : (PORTD = PORTD & ~0x10);
  (dataByte & 0x02) ? (PORTD = PORTD | 0x08) : (PORTD = PORTD & ~0x08);
  (dataByte & 0x01) ? (PORTD = PORTD | 0x04) : (PORTD = PORTD & ~0x04);
}
//-------------------------------------------
unsigned char ReadDataFromPort()
{
  unsigned char dataByte = 0;
  dataByte = 0;
  PORTB |= 0x03;  //Bit 0..1
  if (PINB & 0x02) dataByte |= 0x80;
  if (PINB & 0x01) dataByte |= 0x40;
  PORTD |= ~0x03;                   //internal PULL-UPS set Bit 2..7
  if (PIND & 0x80) dataByte |= 0x20;
  if (PIND & 0x40) dataByte |= 0x10;
  if (PIND & 0x20) dataByte |= 0x08;
  if (PIND & 0x10) dataByte |= 0x04;
  if (PIND & 0x08) dataByte |= 0x02;
  if (PIND & 0x04) dataByte |= 0x01;
  return dataByte;
  //return 0xFE;
}

//RESET 89c2051
//RST = LO  ** ARDUINO-PB3
//XTal1=LO  ** ARDUINO-PC4
//RST   _/-
//XTal1 _/-\__/--\__ (2 times because two mashine cycles are requiwert
//==>Addresscounter =0x000
void ResetAT89c2051()
{
  PORTB &= ~BIT3; //BIT3
  PORTC &= ~BIT4; //BIT4 LOW
  //if(PINC & BIT5) Serial.println("\nNot BUSSY"); else Serial.println("\nBUSSY");
  delay(1);
  PORTB |= BIT3;
  PORTC |= BIT4;    //HIGH
  delay(1);
  PORTC &= ~BIT4;   //LOW
  delay(1);
  PORTC |= BIT4;    //HIGH
  //if(PINC & BIT5) Serial.println("Not BUSSY"); else Serial.println("\nBUSSY");
  delay(1);
  if (PINC & BIT5) Serial.println("READY-RESET"); else Serial.println("\nBUSSY-RESET");
  PORTC &= ~0x10;   //keep it L it's high active
  AT89cXXADDRESSpointer=0;
}

//XTal1 = PC4
//_____/---\___ Adresscounter++
//
void AT89c2051ADRESScounterUP()
{
  AT89cXXADDRESSpointer++;
  PORTC |= 0x10;
  delay(1);
  PORTC &= ~0x10;
  delay(1);
}

void AT89c2051PROG_toggle()
{
  //toggle PRG H-L(1,2mS)-H
  PORTB &= ~BIT4;
  delay(2);
  PORTB |= BIT4;
  delay(1);
}

//MODES           /prog
//          RST   P3.2    P3.3  P3.4  P3.5  P3.7
//Write     12V   H-L-H   L     H     H     H
//Read      H     H       L     L     H     H
//Lock b1   12V   H-L-H   H     H     H     H
//Lock b2   12V   H-L-H   H     H     L     L
//Erase     12V   H-L-H   H     L     L     L
//Read signature
//          H     H     L     L     L     L
//12V ON/OFF over ARDUINO-PB2 High=12V
//AT89c2051-(/bussy P3.1) -- ARDUINO-PC5
//ModeVal=0 READ
//ModeVal=1 WRITE
//ModeVal=2 READ SIGNATURE  ==> ! TWO BYTE-VALUE !
//ModeVal=3 CHIP ERASE      ==> CANCELS even LOCKs
//ModeVal=4 WRITE LOCK-BIT-1 ==> READ-ONLY
//ModeVal=5 WRITE LOCK-BIT-2 ==> READ-WRITE-PROTECTED
void SetAT89c2051Modes(unsigned char ModeVal)
{
  char hexcode[3];
  unsigned char temphexval, tempval;
  switch (ModeVal)
  {
    case 0:
      //          RST   P3.2    P3.3  P3.4  P3.5  P3.7
      //Read      H     H       L     L     H     H
      PORTB &= ~0x04; //NO 12V on RST
      PORTB |= 0x08;  //RST-LOGIC-HIGH
      PORTC &= ~0x08;  //P3.3 L
      PORTB &= ~0x20; //P3.4 L (enable)
      PORTC |= 0x04;  //P3.5 H
      PORTC |= BIT1;  //P3.7 H
      PORTB |= BIT4;  //P3.2 H No TOGGLE
      break;
    case 1:
      //          RST   P3.2    P3.3  P3.4  P3.5  P3.7
      //Write     12V   H-L-H   L     H     H     H
      PORTB |= 0x04; //12V on RST
      PORTB |= 0x08;  //RST-LOGIC-HIGH
      PORTC &= ~0x08;  //P3.3 L
      PORTB |= 0x20;  //P3.4 H (enable)
      PORTC |= 0x04;  //P3.5 H
      PORTC |= 0x02;  //P3.7 H
      //PORTB |= 0x10;  //P3.2 H No TOGGLE
      //AT89c2051PROG_toggle();
      break;
    case 2:
      //        RST   P3.2    P3.3  P3.4  P3.5  P3.7
      //Read signature
      //          H     H     L     L     L     L
      ResetAT89c2051();
      delay(1);
      PORTB &= ~0x04; //NO 12V on RST
      PORTB |= 0x08;  //RST-LOGIC-HIGH
      PORTC &= ~0x08;  //P3.3 L
      PORTB &= ~0x20;  //P3.4 L (enable)
      PORTC &= ~0x04;  //P3.5 L
      PORTC &= ~0x02;  //P3.7 L
      PORTB |= 0x10;  //P3.2 H No TOGGLE
      delay(1000);
      if (PINC & BIT5) Serial.println("\nNot BUSSY MODE-SIG"); else Serial.println("\nBUSSY MODE-SIG");
      tempval = ReadDataFromPort();
      hexcode[0] = nibble2hex(tempval / 16);
      hexcode[1] = nibble2hex(tempval & 0x0F);
      hexcode[2] = 0;
      Serial.print("\nSignature: 0x");
      Serial.print(hexcode);
      AT89c2051ADRESScounterUP();
      delay(1);
      tempval = ReadDataFromPort();
      hexcode[0] = nibble2hex(tempval / 16);
      hexcode[1] = nibble2hex(tempval & 0x0F);
      Serial.print(hexcode);
      AT89c2051ADRESScounterUP();
      delay(1);
      tempval = ReadDataFromPort();
      hexcode[0] = nibble2hex(tempval / 16);
      hexcode[1] = nibble2hex(tempval & 0x0F);
      Serial.print(hexcode);Serial.print(" *");
      if(AT89CXXchip==1) Serial.print("_89_1051 *");
      if(AT89CXXchip==2) Serial.print("_89_2051 *");
      if(AT89CXXchip==4) Serial.print("_89_4051 *");
      Serial.print("\n");
      break;
    case 3:
      break;
    case 4:
      break;
    case 5:
      break;
    default:
      break;
  }
}


//[:] [DATA-count] [Start Address] [Record Type] [DATA] [Checksum]
void AT89cReadDATA()
{
  /*unsigned char AT89cXXADDRESS[2];  // two byte address buffer
  unsigned char AT89cXXRAM[8];      // eight byte RAM buffer
  int AT89cXXADDRESSpointer;
  AT89CXXchip*1024 RAM-SIZE
  */
  char message;
  unsigned char DATAamount, rawDATA, CHECKSumm;
  int ii;
  DATAamount = 8;
  ResetAT89c2051();
  SetDataDirection(0);
  delay(1);
  SetAT89c2051Modes(0);  //0 = RAED-MODE
  delay(1);
  //read DATA : COUNT ADDRESS DATA CHECKSUM
  //while reading and writing DATA send XON/XOFF
  //AT89c2051ADRESScounterUP()
  //message = Serial.read();
  //:03000000020800F3
  //:03000000020800F3
  //AT89CXXchip*1024 RAM-SIZE
  for(AT89cXXADDRESSpointer=0;AT89cXXADDRESSpointer<0x40;/*AT89cXXADDRESSpointer++*/)
  {
  Serial.print(':');
  Serial.print(nibble2hex(DATAamount/16));Serial.print(nibble2hex(DATAamount&0x0F));  // X BYTE of DATA to send
  CHECKSumm=DATAamount;
  /*
  Serial.print(nibble2hex(0));Serial.print(nibble2hex(0));  // 2 BYTE (4 character) of ADDRESS
  CHECKSumm+=0;
  Serial.print(nibble2hex(0));Serial.print(nibble2hex(0));
  CHECKSumm+=0;
  */
  //rawDATA=AT89cXXADDRESS[0];
  rawDATA=AT89cXXADDRESSpointer/256;
  Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));  // 2 BYTE (4 character) of ADDRESS
  CHECKSumm+=rawDATA;
  //rawDATA=AT89cXXADDRESS[1];
  rawDATA=AT89cXXADDRESSpointer&0xFF;
  Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));
  CHECKSumm+=rawDATA;
  rawDATA=0;
  Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));  // TYPE of DATA (4 character) to send
  CHECKSumm+=rawDATA;
  //
  //rawDATA=0x02;
  //Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));
  //CHECKSumm+=rawDATA;
  //rawDATA=0x08;
  //Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));
  //CHECKSumm+=rawDATA;
  //rawDATA=0x00;
  //Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));
  //CHECKSumm+=rawDATA;
  //
  for(ii=0; ii<DATAamount;ii++)
  {
    //AT89cXXADDRESSpointer++;
    AT89c2051ADRESScounterUP();
    //rawDATA=AT89cXXRAM[ii];
    rawDATA=ReadDataFromPort();
    Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));
    CHECKSumm+=rawDATA;
  }
  //CHECKSUMM
  rawDATA= 0x100 - CHECKSumm;
  Serial.print(nibble2hex(rawDATA/16));Serial.print(nibble2hex(rawDATA&0x0F));
  Serial.print('\n');
  }
  Serial.print(":00000001FF\n");
}

void AT89cPREPBurnDATA()
{
  while(Serial.available() > 0) AT89cBurnDATA();
}

void AT89cBurnDATA()
{
  char message;
  unsigned char rawDATA, CHECKSumm;
  int ii, iiEND;
  int DATAamount, AT89cXXADDRESSpTEMP;
  ResetAT89c2051();
  SetDataDirection(1);  //1=DATA-OUT 0=DATA-IN
  delay(1);
  SetAT89c2051Modes(1);  //1 = PROGRAM-MODE
  delay(1);
  //read DATA : COUNT ADDRESS DATA CHECKSUM
  //while reading and writing DATA send XON/XOFF
  //AT89c2051PROG_toggle();
  //AT89c2051ADRESScounterUP()
 if(Serial.available() > 0)
  {
    message = Serial.read();
    if (message == ':') Serial.print("\nOK - CAN GO ON\n");
    else Serial.print("\nNOK - DATA CORRUPT\n");
    //
    message = Serial.read();          //DATA amount
    DATAamount = hex2nibble(message) * 16;
    message = Serial.read();
    DATAamount += hex2nibble(message);
    Serial.print(DATAamount);
    CHECKSumm = DATAamount;
    Serial.print("-C-"); Serial.print(CHECKSumm);
    message = Serial.read();            //high address-BYTE
    rawDATA  = hex2nibble(message) * 16;
    message = Serial.read();
    rawDATA += hex2nibble(message);
    AT89cXXADDRESS[0] = rawDATA;
    //AT89cXXADDRESSpointer = 256 * rawDATA;
    AT89cXXADDRESSpTEMP = 256 * rawDATA;
    CHECKSumm += rawDATA;
    Serial.print("-H-"); Serial.print(CHECKSumm);
    //
    message = Serial.read();            //low address-BYTE
    rawDATA  = hex2nibble(message) * 16;
    message = Serial.read();
    rawDATA += hex2nibble(message);
    AT89cXXADDRESS[1] = rawDATA;
    //AT89cXXADDRESSpointer += rawDATA;
    AT89cXXADDRESSpTEMP += rawDATA;
    CHECKSumm += rawDATA;
    Serial.print("-L-"); Serial.print(CHECKSumm);
    //
    message = Serial.read();            // DATA TYPE
    rawDATA  = hex2nibble(message) * 16;
    message = Serial.read();
    rawDATA += hex2nibble(message);
    CHECKSumm += rawDATA;
    Serial.print("-T-"); Serial.print(CHECKSumm);
    Serial.print("--adjust ADDRESS--");
    AT89cXXADDRESSpTEMP   &= 0x0fff;
    AT89cXXADDRESSpointer &= 0x0fff;
    if(AT89cXXADDRESSpTEMP - AT89cXXADDRESSpointer != 0)
    {
      Serial.print(AT89cXXADDRESSpTEMP);Serial.print("-_-");Serial.print(AT89cXXADDRESSpointer);
      while(AT89cXXADDRESSpTEMP - AT89cXXADDRESSpointer)
      {
        AT89c2051ADRESScounterUP();
      }
    }
    //---DATAWRITELOOP
    do{
      //DATAamunt can vary between 0 and 256 but in one rush just write EIGHT bytes
      for (ii = 0; ii < DATAamount && ii<8; ii++)
      {
        message = Serial.read();
        rawDATA = hex2nibble(message) * 16;
        message = Serial.read();
        rawDATA += hex2nibble(message);
        AT89cXXRAM[ii] = rawDATA;
        CHECKSumm += rawDATA;
        Serial.print("-DL-"); Serial.print(CHECKSumm);
      }
    
      //if(Serial.available()>0) message = Serial.read();
      //if(message=='\r') break;
      Serial.print(XOFF);
      //now write it to the chip
      //AT89cXXADDRESSpTEMP - AT89cXXADDRESSpointer != 0
        if(DATAamount < 8)
          iiEND =DATAamount;
        else
          iiEND =8;
        for(ii=0; ii<iiEND;ii++)
        {
          PutData2Port(AT89cXXRAM[ii]);
          AT89c2051PROG_toggle();
          AT89c2051ADRESScounterUP();
        }
      DATAamount -=8; //z.b. 256 - 8...
      Serial.print(XON);
      }while(DATAamount>0);
  }
  //
    message = Serial.read();  //READCHECKSumm High and Low
    message = Serial.read();
    message = Serial.read();  //READ BS-N
    /*
    while (Serial.available() > 0)
    {
      message = Serial.read();
      if (message == '\n') Serial.print("BS-N");
      if (message == '\r') Serial.print("BS-R");
      if (message != '\r' && message != '\n') 
      {
        Serial.print(nibble2hex(message / 16));
        Serial.print(nibble2hex(message & 0x0F));
      }
    }
    */
  //CHECKSumm = CHECKSumm & 0xFF;
  CHECKSumm = (256 - CHECKSumm);
  Serial.print("\n-CHECKSumm--");

  Serial.print(nibble2hex(CHECKSumm / 16));
  Serial.print(nibble2hex(CHECKSumm & 0x0F));
  Serial.print("\n-/CHECKSumm--");
  SetDataDirection(0);  //PORT set it to input
}

// the loop routine runs over and over again forever:
void loop()
{
  char hexcode[3], temphexval;
  unsigned char tempval;
  int ii;
  // read the input on analog pin 0:
  //int sensorValue = analogRead(A0)/4;
  char sensorValue = PIND;
  char message;

  if (Serial.available() > 0)
  {
    message = Serial.read();
    Serial.write(XOFF);
    Serial.write(message);
    Serial.write(XON);

    if (message == '\n') {
      Serial.print("BS-N");
    }
    if (message == '?') {
      SendBurnerMenue();
    }
    if (message == 'W') 
    {
      //AT89cBurnDATA();
      AT89cPREPBurnDATA();
    }
    if (message == 'R') {
      AT89cReadDATA();
    }
    if(message=='V')    //read SIGNATURE
    {
      SetDataDirection(0); // 1 OUTput 0 INput
      delay(1);        // delay in between reads for stability
      SetAT89c2051Modes(2); //2 READ signature
      //
    }
    while (Serial.available() > 0) Serial.read();
  }
  message = ' ';
  //while(Serial.available()>0) Serial.read();
  /*while(Serial.available()>0)
    {
      //XOFF=0x13 XON=0x11
      message = Serial.read();
      Serial.write(XOFF);
      Serial.write(message);
      Serial.write(XON);
    }*/


  //temphexval = 15;
  //if(temphexval<10)
  //{
  //  temphexval += '0';
  //}
  //else temphexval += 'A'-10;
  hexcode[0] = nibble2hex(sensorValue / 16);
  hexcode[1] = nibble2hex(sensorValue & 0x0F);
  hexcode[2] =0;
  // print out the value you read:
  Serial.print(sensorValue);
  Serial.print(" 0x");
  Serial.println(hexcode);
  /*
  SetDataDirection(0); // 1 OUTput 0 INput
  delay(1);        // delay in between reads for stability
  SetAT89c2051Modes(2); //2 READ signature
  */
  delay(1);
  //---READ TEST
  ResetAT89c2051();
  SetAT89c2051Modes(0); //0 READ DATA
  delay(1000);
  Serial.print("\nDATA at 0x000 ");
  for (ii = 0; ii < 8; ii++)
  {
    tempval = ReadDataFromPort;
    hexcode[0] = nibble2hex(tempval / 16);
    hexcode[1] = nibble2hex(tempval & 0x0F);
    Serial.print(hexcode);
    Serial.print(" ");
    AT89c2051ADRESScounterUP;
    delay(1);
  }
  Serial.print("\n\r");
  Serial.print("DATA at 0x008 ");
  for (ii = 0; ii < 8; ii++)
  {
    tempval = ReadDataFromPort;
    hexcode[0] = nibble2hex(tempval / 16);
    hexcode[1] = nibble2hex(tempval & 0x0F);
    Serial.print(hexcode);
    Serial.print(" ");
    AT89c2051ADRESScounterUP;
    delay(1);
  }
  Serial.print("\n");
}
