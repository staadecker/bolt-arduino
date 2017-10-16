#include "bluetooth.h"

#include "const.h"
#include "led.h"
#include "logger.h"

#include <SoftwareSerial.h>
#include <Thread.h>

namespace bluetooth {
  namespace {
    SoftwareSerial mySerial(constants::P_SOFTWARE_SERIAL_RX, constants::P_SOFTWARE_SERIAL_TX);

    const char START_OF_PACKET = 0x02;  //Start of text
    const char END_OF_PACKET = 0x03;  //End of text
    const char ACKNOWLEDGE = 0x06; //Acknowledge

    const String REQUEST_PACKET_CONTENT = "R";

    const char C_END = 0x45; //"D"
    const char C_START = 0x53; //"S"
    const char C_TURN_ON_LED = 0x4C; //"L"
    const char C_TURN_OFF_LED = 0x4F; //"O"

    const unsigned long PACKET_TIMEOUT = 1000;
    const int TEST_CONNECTION_INTERVAL = 2000;

    const String BLUETOOTH_PIN = "756945";

    bool goOnline = false;

    

    void acknowledgePacket() {
      mySerial.print(ACKNOWLEDGE);
    }

    void processPacketContent(String packetContent) {
      char command = packetContent[0];
      String argument = packetContent.substring(1);

      switch (command) {
        case C_END:
          goOnline = false;
          break;
        case C_START:
          goOnline = true;
          break;
        case C_TURN_ON_LED:
          led::turnOn(argument.toInt());
          break;
        case C_TURN_OFF_LED:
          led::turnOff(argument.toInt());
          break;
        default:
          logger::log(logger::TYPE_ERROR, "bluetooth", "can't parse packet : " + packetContent);
      }
    }

    String getPacketContent() {
      String packet = "";

      unsigned long timeAtLastByte = millis();

      while (millis() < timeAtLastByte + PACKET_TIMEOUT) {
        if (Serial.available()) {

          char newByte = Serial.read();
          timeAtLastByte = millis();

          if (newByte == END_OF_PACKET) {
            return packet;
          } else {
            packet += newByte;
          }
        }
      }
      logger::log(logger::TYPE_WARNING, "bluetooth", "Timout while reading packet content");
      return String(C_END); // If while loop ended because off timeout, end game
    }
  }

  void readReceived() {
    String unknown = "";

    while (Serial.available()) {
      char newByte = Serial.read();

      switch (newByte) {
        case ACKNOWLEDGE:
          //TODO
          break;
        case START_OF_PACKET:
          processPacketContent(getPacketContent());
          acknowledgePacket();
          break;
        default:
          unknown += newByte;
      }
    }

    if (not unknown.equals("")) {
      logger::log(logger::TYPE_WARNING, "bluetooth", "Unknown RX bytes : " + unknown);
    }
  }


  void listen() {
    while (goOnline) {
      readReceived();
    }
  }

  void sendPacket(String packetContent) {
    mySerial.print(START_OF_PACKET + packetContent + END_OF_PACKET);
  }

  bool shouldGoOnline() {
    return goOnline;
  }

  void setup() {
    mySerial.begin(9600);
    mySerial.print("AT+PIN:" + BLUETOOTH_PIN); //Set PIN to be same as phone
  }
}
