#include <Console.h>
#include <SoftwareSerial.h>
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>
#include <FileIO.h>
#include <Temboo.h>
#include "TembooAccount.h" // contains Temboo account information

BridgeServer server;
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

    if(verbose) dbgWriteArray(SendData, sizeof(SendData));
    clearReceiveData();

    for (int i = 0; i < MaxAttempt; i++)
    {
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
  byte ReceiveData[8];

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
    float Valore;
    bool ReadState;
  } DataDSP;

  DataDSP DSP;

  bool ReadDSP(byte type, byte global) {
    if ((((int)type >= 1 && (int)type <= 9) || ((int)type >= 21 && (int)type <= 63)) && ((int)global >= 0 && (int)global <= 1)) {
      DSP.ReadState = Send(Address, (byte)59, type, global, (byte)0, (byte)0, (byte)0, (byte)0);

      if (DSP.ReadState == false) {
        ReceiveData[0] = 255;
        ReceiveData[1] = 255;
      }
    }
    else {
      DSP.ReadState = false;
      clearReceiveData();
      ReceiveData[0] = 255;
      ReceiveData[1] = 255;
    }

    DSP.TransmissionState = ReceiveData[0];
    DSP.GlobalState = ReceiveData[1];

    foo.asBytes[0] = ReceiveData[5];
    foo.asBytes[1] = ReceiveData[4];
    foo.asBytes[2] = ReceiveData[3];
    foo.asBytes[3] = ReceiveData[2];

    DSP.Valore = foo.asFloat;

    File dF = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
    if (type == 1) {
      dF.print("Grid Voltage: ");
    }
    else if (type == 2) {
      dF.print("Grid Current: ");
    }
    else if (type == 3) {
      dF.print("Grid Power: ");
    }
    else if (type == 5) {
      dF.print("Vbulk: ");
    }
    else if (type == 8) {
      dF.print("Power 1: ");
    }
    else if (type == 9) {
      dF.print("Power 2: ");
    }
    dF.print(DSP.Valore, 2);
    dF.println(" ");
    dF.close();

    return DSP.ReadState;
  }

  typedef struct {
    byte TransmissionState;
    byte GlobalState;
    unsigned long Seconds; // seconds since Jan 1, 2000
    bool ReadState;
  } DataTimeDate;

  DataTimeDate TimeDate;

  bool ReadTimeDate() {
    TimeDate.ReadState = Send(Address, (byte)70, (byte)0, (byte)0, (byte)0, (byte)0, (byte)0, (byte)0);

    if (TimeDate.ReadState == false) {
      ReceiveData[0] = 255;
      ReceiveData[1] = 255;
    }

    TimeDate.TransmissionState = ReceiveData[0];
    TimeDate.GlobalState = ReceiveData[1];
    TimeDate.Seconds = ((unsigned long)ReceiveData[2] << 24) + ((unsigned long)ReceiveData[3] << 16) + ((unsigned long)ReceiveData[4] << 8) + (unsigned long)ReceiveData[5];
    return TimeDate.ReadState;
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

    File dF = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
  
    dF.print("NRG: ");
    dF.print(CumulatedEnergy.Energy, DEC);
    dF.println(" ");
    dF.close();
    return CumulatedEnergy.ReadState;
  }
};

clsAurora Inverter = clsAurora(2);
unsigned long lastLog;
unsigned long currLog;
const unsigned long logInterval = 60000; //loggin interval in ms
int numRuns = 1;   // execution count, so this doesn't run forever
int maxRuns = 100; // max number of times the Google Spreadsheet Choreo should run

void setup() {
  // initialize communications
  Bridge.begin();
  mySerial.begin(19200);
  Console.begin();
  while(!Console);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  File dF = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
  dF.println("hello");
  dF.close();
  
  //Initialize timers
  lastLog = millis();  
  currLog = lastLog;

  // initialize the LED pin and SWSerial port as an output:
  pinMode(ledPin, OUTPUT);
  pinMode(SSerialTxControl, OUTPUT);
  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver   
}

void loop() {
  
  // Get clients coming from server
  BridgeClient client = server.accept();
  
  // There is a new client?
  if (client) {
    process(client);
    client.stop();
  }

  // Delay logInterval (60s) until calc Nrg
  if(currLog - lastLog <= logInterval){
    currLog = millis();
  }
  else{
    lastLog = millis();
    logNrg();
  }
}

void process(BridgeClient client) {
  // read the command
  String command = client.readStringUntil('/');

  // is "status" command?
  if (command == "status") {
    statusCommand(client);
  }

  File dF = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
  dF.println("End of CMD " + command);
  dF.close();
}

void statusCommand(BridgeClient client) {
  int request, value;
  bool verb = false;
  // Read request number
  request = client.parseInt();
  if (request ==2 ) verb = true;

  // If the next character is a '/' it means we have an URL
  // with a value like: "/status/1/1"
  if (client.read() == '/') {
    value = client.parseInt();
  }

  File dF = FileSystem.open("/usr/ralton/datalog.txt", FILE_APPEND);
  dF.print("Status: ");
  dF.print(request, DEC);
  dF.println(" ");

  //Check if there is anything in the SW Ser buffer and clear it
  value = mySerial.available();
  if (value > 0){
    
    if(verb) dF.print("Data in buffer: ");
    if(verb) dF.print(value, DEC);
    if(verb) dF.println(" ");

    while(mySerial.available()){
      mySerial.read();
    }

    if(verb) value = mySerial.available();
    if(verb) dF.print("Data left: ");
    if(verb) dF.print(value, DEC);
    if(verb) dF.println(" ");
  }
  dF.close();

  if(verb) Inverter.ReadDSP(1,0);
  if(verb) Inverter.ReadDSP(2,0);
  if(verb) Inverter.ReadDSP(3,0);
  if(verb) Inverter.ReadDSP(5,0);
  if(verb) Inverter.ReadDSP(8,0);
  if(verb) Inverter.ReadDSP(9,0);
  Inverter.ReadCumulatedEnergy(5);
}

void logNrg(){

  Inverter.ReadCumulatedEnergy(5);
  Inverter.ReadTimeDate();
  Console.print("Seconds: "); Console.println(Inverter.TimeDate.Seconds);
  Console.print("Energy:  "); Console.print(Inverter.CumulatedEnergy.Energy); Console.println(" Wh");

  // Limit number of runs...
  if (numRuns <= maxRuns) {

    // Process object to send a Choreo request to Temboo
    TembooChoreo AppendValuesChoreo;
    AppendValuesChoreo.begin();
    
    // set Temboo account credentials
    AppendValuesChoreo.setAccountName(TEMBOO_ACCOUNT);
    AppendValuesChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
    AppendValuesChoreo.setAppKey(TEMBOO_APP_KEY);
    
    // identify the Temboo Library choreo to run (Google > Sheets > AppendValues)
    AppendValuesChoreo.setChoreo("/Library/Google/Sheets/AppendValues");
    
    // set the required Choreo inputs
    // see https://www.temboo.com/library/Library/Google/Sheets/AppendValues/ 
    // for complete details about the inputs for this Choreo
    
    // setup Google access
    AppendValuesChoreo.addInput("ClientID", GOOGLE_CLIENT_ID);
    AppendValuesChoreo.addInput("ClientSecret", GOOGLE_CLIENT_SECRET);
    AppendValuesChoreo.addInput("RefreshToken", GOOGLE_REFRESH_TOKEN);
    AppendValuesChoreo.addInput("SpreadsheetID", SPREADSHEET_ID);

    // convert the time and sensor values to a json array
    String rowData = "[[\"" + String(Inverter.TimeDate.Seconds) + "\", \"" + String(Inverter.CumulatedEnergy.Energy) + "\"]]";

    // add the RowData input item
    AppendValuesChoreo.addInput("Values", rowData);

    // run the Choreo and wait for the results
    // The return code (returnCode) will indicate success or failure 
    unsigned int returnCode = AppendValuesChoreo.run();

    // return code of zero (0) means success
    if (returnCode == 0) {
      Console.println("Success! Appended " + rowData);
      Console.println("");
    } else {
      // return code of anything other than zero means failure  
      // read and display any error messages
      while (AppendValuesChoreo.available()) {
        char c = AppendValuesChoreo.read();
        Console.print(c);
      }
    }
    AppendValuesChoreo.close();
  }
}



