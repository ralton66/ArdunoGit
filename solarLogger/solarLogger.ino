#include <Console.h>
#include <SoftwareSerial.h>
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>
#include <FileIO.h>

SoftwareSerial mySerial(8,3);
const int ledPin = 13; // the pin that the LED is attached to
bool verbose = false;

//RS485 control
#define SSerialTxControl 4 
#define RS485Transmit HIGH 
#define RS485Receive LOW 

void dbgWriteArray(byte* a, int size){

    File dataFile = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
  
    // if the file is available, write to it:
    if (dataFile) {
      for (int i = 0; i < size; i++){
        dataFile.print(a[i]);
        dataFile.print(" ");
      }
      dataFile.println(" ");
      dataFile.close();
    }
}

void dbgWrite(String s){

    File dataFile = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
  
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(s);
      dataFile.close();
    }
}

class clsAurora {
private:
  int MaxAttempt = 1;
  byte Address = 0;
  int idx = 0;
  unsigned long start;  //some global variables available anywhere in the program
  unsigned long curr;
  const unsigned long msPrd = 5;  //the value is a number of milliseconds

  void clearData(byte *data, byte len) {
    for (int i = 0; i < len; i++) {
      data[i] = 0;
    }
  }

  int Crc16(byte *data, int offset, int count)
  {
    byte BccLo = 0xFF;
    byte BccHi = 0xFF;

    for (int i = offset; i < (offset + count); i++)
    {
      byte New = data[offset + i] ^ BccLo;
      byte Tmp = New << 4;
      New = Tmp ^ New;
      Tmp = New >> 5;
      BccLo = BccHi;
      BccHi = New ^ Tmp;
      Tmp = New << 3;
      BccLo = BccLo ^ Tmp;
      Tmp = New >> 4;
      BccLo = BccLo ^ Tmp;
    }
    return (int)word(~BccHi, ~BccLo);
  }

  bool Send(byte address, byte param0, byte param1, byte param2, byte param3, byte param4, byte param5, byte param6)
  {
    SendStatus = false;
    ReceiveStatus = false;

    byte SendData[10];
    SendData[0] = address;
    SendData[1] = param0;
    SendData[2] = param1;
    SendData[3] = param2;
    SendData[4] = param3;
    SendData[5] = param4;
    SendData[6] = param5;
    SendData[7] = param6;

    int crc = Crc16(SendData, 0, 8);
    SendData[8] = lowByte(crc);
    SendData[9] = highByte(crc);

    if(verbose) dbgWrite("TX Data:");;
    if(verbose) dbgWriteArray(SendData, sizeof(SendData));
    clearReceiveData();

    for (int i = 0; i < MaxAttempt; i++)
    {
      // Flush input buffer
      while (mySerial.available()){
          mySerial.read();
      }
      
      digitalWrite(SSerialTxControl, RS485Transmit);
      delay(50);
      int numBytes = 0;
      int timeOut = 0;

      if (mySerial.write(SendData, sizeof(SendData)) != 0) {
        mySerial.flush();
        SendStatus = true;

        digitalWrite(SSerialTxControl, RS485Receive);

        // Wait for first byte of response
        while (!mySerial.available()){
        }

        // Delay msPrd (5ms) for rest of packet
        start = millis();  
        curr = start;
        while(curr - start <= msPrd){
          curr = millis();
        }  
        
        idx = 0;
        while ((mySerial.available()) && (idx < 32)){
          ReceiveData[idx++] = mySerial.read();
        }
        if ((int)word(ReceiveData[7], ReceiveData[6]) == Crc16(ReceiveData, 0, 6)) {
          if(verbose) dbgWrite("ok");
          ReceiveStatus = true;
        }
        if(verbose) dbgWriteArray(ReceiveData, sizeof(ReceiveData));
      }
    }
    return ReceiveStatus;
  }

  union {
    byte asBytes[4];
    float asFloat;
  } foo;

  union {
    byte asBytes[4];
    unsigned long asUlong;
  } ulo;

public:
  bool SendStatus = false;
  bool ReceiveStatus = false;
  byte ReceiveData[32];

  clsAurora(byte address) {
    Address = address;
    SendStatus = false;
    ReceiveStatus = false;
    clearReceiveData();
  }

  void clearReceiveData() {
    clearData(ReceiveData, 32);
  }

  typedef struct {
    byte TransmissionState;
    byte GlobalState;
    unsigned long Energy; // Whrs
    bool ReadState;
  } DataCumulatedEnergy;

  DataCumulatedEnergy CumulatedEnergy;

  bool ReadCumulatedEnergy(byte par) {
    if ((int)par >= 0 && (int)par <= 6) {
      CumulatedEnergy.ReadState = Send(Address, (byte)78, par, (byte)0, (byte)0, (byte)0, (byte)0, (byte)0);

      if (CumulatedEnergy.ReadState == false) {
        ReceiveData[0] = 255;
        ReceiveData[1] = 255;
      }
    }
    else {
      CumulatedEnergy.ReadState = false;
      clearReceiveData();
      ReceiveData[0] = 255;
      ReceiveData[1] = 255;
    }

    CumulatedEnergy.TransmissionState = ReceiveData[0];
    CumulatedEnergy.GlobalState = ReceiveData[1];
    if (CumulatedEnergy.ReadState == true) {
        ulo.asBytes[0] = ReceiveData[5];
        ulo.asBytes[1] = ReceiveData[4];
        ulo.asBytes[2] = ReceiveData[3];
        ulo.asBytes[3] = ReceiveData[2];
        CumulatedEnergy.Energy = ulo.asUlong;
    }
    if(verbose) {
      File dF = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
      dF.print("NRG: ");
      dF.print(CumulatedEnergy.Energy, DEC);
      dF.println(" ");
      dF.close();
    }
    return CumulatedEnergy.ReadState;
  }
};

clsAurora Inverter = clsAurora(2);
unsigned long lastLog;
unsigned long currLog;
const unsigned long logInterval = 61000; //loggin interval in ms
int numRuns = 1;   // execution count, so this doesn't run forever
int maxRuns = 2; // max number of times the Google Spreadsheet Choreo should run
int cnt = 0;
bool stat;

void setup() {
  // initialize communications
  Bridge.begin();
  mySerial.begin(19200);
  Console.begin();
  //while(!Console);
  //Console.println("Solar Logger");

  //File dF = FileSystem.open("/mnt/sda1/data.txt", FILE_APPEND);
  //dF.println("hello");
  //dF.close();
  
  //Initialize timers
  lastLog = millis();  
  currLog = lastLog;

  // initialize the LED pin and SWSerial port as an output:
  pinMode(ledPin, OUTPUT);
  pinMode(SSerialTxControl, OUTPUT);
  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver   
  Inverter.ReadCumulatedEnergy(5);
}

void loop() {
  // Delay logInterval (60s) until calc Nrg
  if(currLog - lastLog <= logInterval){
    currLog = millis();
  }
  else{
    stat = Inverter.ReadCumulatedEnergy(5);
    Console.println((Inverter.CumulatedEnergy.Energy));
    lastLog = millis();
    currLog = lastLog;
    if((cnt++ > 15)&& stat){
      cnt = 0;
      String rowData = String(Inverter.CumulatedEnergy.Energy) + ", " + String(lastLog) +",";
      //String rowData = "[[\"" + String(cnt) + "\"]]";
      File dF = FileSystem.open("/mnt/sda1/data.txt", FILE_APPEND);
      dF.println(rowData);
      dF.close();
    }
  }
}


