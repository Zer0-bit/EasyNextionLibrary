/*!
 * EasyNextionLibrary.cpp - Easy library for Nextion Displays
 * Copyright (c) 2020 Athanasios Seitanis < seithagta@gmail.com >.
 * All rights reserved under the library's licence
 */

// include this library's description file
#ifndef EasyNextionLibrary_h
#include "EasyNextionLibrary.h"
#endif

#ifndef trigger_h
#include "trigger.h"
#endif

//-------------------------------------------------------------------------
 // Constructor : Function that handles the creation and setup of instances
//---------------------------------------------------------------------------

EasyNex::EasyNex(HardwareSerial& serial){  // Constructor's parameter is the Serial we want to use
  _serial = &serial;
}

void EasyNex::begin(unsigned long baud){
  _serial->begin(baud);  // We pass the initialization data to the objects (baud rate) default: 9600

  delay(100);            // Wait for the Serial to initialize

  _tmr1 = millis();
  while(_serial->available() > 0){     // Read the Serial until it is empty. This is used to clear Serial buffer
    if((millis() - _tmr1) > 100UL){    // Reading... Waiting... But not forever......
      break;
    }
      _serial->read();                // Read and delete bytes
  }
}


/*
 * -- writeNum(String, uint32_t): for writing in components' numeric attribute
 * String = objectname.numericAttribute (example: "n0.val"  or "n0.bco".....etc)
 * uint32_t = value (example: 84)
 * Syntax: | myObject.writeNum("n0.val", 765);  |  or  | myObject.writeNum("n0.bco", 17531);       |
 *         | set the value of numeric n0 to 765 |      | set background color of n0 to 17531 (blue)|
 */
void EasyNex::writeNum(String compName, uint32_t val){
	_component = compName;
	_numVal = val;

	_serial->print(_component + "=" + _numVal + _endChars);
}


/*
 * -- writeStr(String, String): for writing in components' text attributes
 * String No1 = objectname.textAttribute (example: "t0.txt"  or "b0.txt")
 * String No2 = value (example: "Hello World")
 * Syntax: | myObject.writeStr("t0.txt", "Hello World");  |  or  | myObject.writeNum("b0.txt", "Button0"); |
 *         | set the value of textbox t0 to "Hello World" |      | set the text of button b0 to "Button0"  |
 */
void EasyNex::writeStr(String command, String txt){
	_component = command;
	_strVal = txt;

  if(_strVal == "cmd")
    _serial->print(_component + _endChars);
  else if(_strVal != "cmd")
    _serial->print(_component + "=\"" + _strVal + "\"" + _endChars);
}

String EasyNex::readStr(String TextComponent){

  String _Textcomp = TextComponent;
  bool _endOfCommandFound = false;
  char _tempChar;

    _tmr1 = millis();
  while(_serial->available()){                  // Waiting for NO bytes on Serial,
                                                    // as other commands could be sent in that time.
    if((millis() - _tmr1) > 100UL){                // Waiting... But not forever...after the timeout
      _readString = "ERROR";
      break;                                // Exit from the loop due to timeout. Return ERROR
    }else{
      NextionListen();                      // We run NextionListen(), in case that the bytes on Serial are a part of a command
    }
  }

  // As there are NO bytes left in Serial, which means no further commands need to be executed,
  // send a "get" command to Nextion
  _serial->print("get " + _Textcomp + _endChars); // The String of a component you want to read on Nextion

  // And now we are waiting for a reurn data in the following format:
  // 0x70 ... (each character of the String is represented in HEX) ... 0xFF 0xFF 0xFF

  // Example: For the String ab123, we will receive: 0x70 0x61 0x62 0x31 0x32 0x33 0xFF 0xFF 0xFF

  _tmr1 = millis();
  while(_serial->available() < 4){                  // Waiting for bytes to come to Serial, an empty Textbox will send 4 bytes
                                                    // and this the minimmum number that we are waiting for (70 FF FF FF)
    if((millis() - _tmr1) > 100UL){                // Waiting... But not forever...after the timeout
       _readString = "ERROR";
       break;                        // Exit the loop due to timeout. Return ERROR
    }
  }

  if(_serial->available() > 3){         // Read if more then 3 bytes come (we always wait for more than 3 bytes)
    _start_char = _serial->read();      //  variable (start_char) read and store the first byte on it
    _tmr1 = millis();

    while(_start_char != 0x70){      // If the return code 0x70 is not detected,
      if(_serial->available()){      // read the Serial until you find it
        _start_char = _serial->read();
      }

      if((millis() - _tmr1) > 100UL){     // Waiting... But not forever......
        _readString = "ERROR";          // If the 0x70 is not found within the given time, break the while()
                                        // to avoid being stuck inside the while() loop
        break;
      }
    }

    if (_start_char == 0x70) {
      _readString = "";
      unsigned long startTime = millis();

      while (!_endOfCommandFound && (millis() - startTime) < 100UL) {
        if (_serial->available()) {
          char tempChar = _serial->read();
          _readString += tempChar;

          if (_readString.length() >= 3 && _readString.substring(_readString.length() - 3) == _endChars) {
            _endOfCommandFound = true;
            _readString.remove(_readString.length() - 3); // Remove the end bytes from the _readString
          }
        }
      }

      if (!_endOfCommandFound) {
        _readString = "ERROR";
      }
    }
  }

  return _readString;
}

/*
 * -- readNumber(String): We use it to read the value of a components' numeric attribute on Nextion
 * In every component's numeric attribute (value, bco color, pco color...etc)
 * String = objectname.numericAttribute (example: "n0.val", "n0.pco", "n0.bco"...etc)
 * Syntax: | myObject.readNumber("n0.val"); |  or  | myObject.readNumber("b0.bco");                       |
 *         | read the value of numeric n0   |      | read the color number of the background of butoon b0 |
 */

uint32_t EasyNex::readNumber(String component){

  _comp = component;
  bool _endOfCommandFound = false;
  char _tempChar;

    _numberValue = _error;                      // The function will return this number in case it fails to read the new number

    _tmr1 = millis();
  while(_serial->available()){                  // Waiting for NO bytes on Serial,
                                                    // as other commands could be sent in that time.
    if((millis() - _tmr1) > 100UL){                // Waiting... But not forever...after the timeout
      _numberValue = _error;
      break;                                // Exit from the loop due to timeout. Return _error
    }else{
      NextionListen();                      // We run NextionListen(), in case that the bytes on Serial are a part of a command
    }
  }


  // As there are NO bytes left in Serial, which means no further commands need to be executed,
  // send a "get" command to Nextion

  _serial->print("get " + _comp + _endChars); // The String of a component you want to read on Nextion

  // And now we are waiting for a reurn data in the following format:
  // 0x71 0x01 0x02 0x03 0x04 0xFF 0xFF 0xFF
  // 0x01 0x02 0x03 0x04 is 4 byte 32-bit value in little endian order.

    _tmr1 = millis();
  while(_serial->available() < 8){                  // Waiting for bytes to come to Serial,
                                                    // we are waiting for 8 bytes
    if((millis() - _tmr1) > 100UL){                // Waiting... But not forever...after the timeout
       _numberValue = _error;
       break;                                  // Exit the loop due to timeout. Return _error
    }
  }

  if(_serial->available() > 7){
    _start_char = _serial->read();      //  variable (start_char) read and store the first byte on it
    _tmr1 = millis();

    while(_start_char != 0x71){      // If the return code 0x71 is not detected,
      if(_serial->available()){      // read the Serial until you find it
        _start_char = _serial->read();
      }

      if((millis() - _tmr1) > 100UL){     // Waiting... But not forever......
        _numberValue = _error;          // If the 0x71 is not found within the given time, break the while()
                                        // to avoid being stuck inside the while() loop
        break;
      }
    }

    if(_start_char == 0x71){  // If the return code 0x71 is detected,

			for(int i = 0; i < 4; i++){   // Read the 4 bytes represent the number and store them in the numeric buffer

        _numericBuffer[i] = _serial->read();
	    }


      int _endBytes = 0;  // This variable helps us count the end command bytes of Nextion. The 0xFF 0xFF 0xFF
      _tmr1 = millis();

      while(_endOfCommandFound == false){  // As long as the three 0xFF bytes have NOT been found, run the commands inside the loop

        _tempChar = _serial->read();  // Read the next byte of the Serial

        if(_tempChar == 0xFF){  // If the read byte is the end command byte,
          _endBytes++ ;      // Add one to the _endBytes counter
          if(_endBytes == 3){
            _endOfCommandFound = true;  // If the counter is equal to 3, we have the end command
          }
        }else{ // If the read byte is NOT the end command byte,
          _numberValue = _error;
          break;
        }

        if((millis() - _tmr1) > 100UL){     // Waiting... But not forever......
          _numberValue = _error;           // If the end of the command is NOT found in the time given,
                                           // break the while
          break;
        }
      }
    }
  }

  _numberValue = _endOfCommandFound
                  ? (_numericBuffer[3] << 24) | (_numericBuffer[2] << 16) | (_numericBuffer[1] << 8) | _numericBuffer[0]
                  : _error;

  return _numberValue;
}

/*
 * -- readByte(): Main purpose and usage is for the custom commands read
 * Where we need to read bytes from Serial inside user code
 */

int  EasyNex::readByte(){

 int _tempInt = _serial->read();

 return _tempInt;

}



/*
 * -- NextionListen(): It uses a custom protocol to identify commands from Nextion Touch Events
 * For advanced users: You can modify the custom protocol to add new group commands.
 * More info on custom protocol: https://seithan.com/Easy-Nextion-Library/Custom-Protocol/ and on the documentation of the library
 */
/*! WARNING: This function must be called repeatedly to response touch events
 * from Nextion touch panel.
 * Actually, you should place it in your loop function.
 */
void EasyNex::NextionListen(){
	if(_serial->available() > 2){         // Read if more then 2 bytes come (we always send more than 2 <#> <len> <cmd> <id>
    _start_char = _serial->read();      // Create a local variable (start_char) read and store the first byte on it
    _tmr1 = millis();

    while(_start_char != '#'){
      _start_char = _serial->read();        // whille the start_char is not the start command symbol
                                           //  read the serial (when we read the serial the byte is deleted from the Serial buffer)
      if((millis() - _tmr1) > 100UL){     //   Waiting... But not forever......
        break;
      }
    }
    if(_start_char == '#'){            // And when we find the character #
      _len = _serial->read();          //  read and store the value of the second byte
                                       // <len> is the lenght (number of bytes following)
      _tmr1 = millis();
      _cmdFound = true;

      while(_serial->available() < _len){     // Waiting for all the bytes that we declare with <len> to arrive
        if((millis() - _tmr1) > 100UL){         // Waiting... But not forever......
          _cmdFound = false;                  // tmr_1 a timer to avoid the stack in the while loop if there is not any bytes on _serial
          break;
        }
      }

      if(_cmdFound == true){                  // So..., A command is found (bytes in _serial buffer egual more than len)
        _cmd1 = _serial->read();              // Read and store the next byte. This is the command group
        readCommand();                        // We call the readCommand(),
                                              // in which we read, seperate and execute the commands
			}
		}
	}
}
