#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <EBYTE22.h>
#include <HardwareSerial.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>

// All pins valid for ESP32 DEVKIT V1
// You can only use the 3.3 volt levels!!!
// To work with Arduino you need to use a level converter

// Lora pins
#define PIN_TX 17
#define PIN_RX 16

// M0:0v M1:0v Normal mode
// M0:0v M1:3v WOR mode
// M0:3v M1:0v Configuration mode
// M0:3v M1:3v Sleep mode
#define PIN_M0 14
#define PIN_M1 12

// 0v on pin: module is busy
// 3v on pin: module is free
#define PIN_AX 13

// Display pins, I use 320x240 2 inch
#define TFT_CS 5
#define TFT_RST 4
#define TFT_DC 2

// I2C pins, if you want to use i2c display
//#define I2C_SDA 21
//#define I2C_SCL 22

Adafruit_ST7789 Display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
EBYTE22 Lora(&Serial1, PIN_M0, PIN_M1, PIN_AX);
BluetoothSerial SerialBluetooth;
Preferences Preferences;
TaskHandle_t TaskUpdateDisplay;
TaskHandle_t TaskSystemProcesses;

const char PROGMEM READY[] = "Ready";
const char PROGMEM COMMAND_SEND[] = "Send ";
const char PROGMEM COMMAND_CHANNEL[] = "Channel ";
const char PROGMEM COMMAND_RATE[] = "Rate ";
const char PROGMEM COMMAND_PACKET[] = "Packet ";
const char PROGMEM COMMAND_RESET[] = "Reset";
const char PROGMEM COMMAND_POWER[] = "Power ";
const char PROGMEM COMMAND_KEY[] = "Key ";
const char PROGMEM COMMAND_NET_ID[] = "NetID ";
const char PROGMEM COMMAND_ADDRESS[] = "Address ";

const char PROGMEM COMMANDS_ALL[] = "Commands:\nSend ... (some text)\nChannel ... (0-83)\nRate ... (300/1200/2400/4800/9600/19200/38400/62500)\nPacket ... (32/64/128/240)\nReset\nPower ... (1/2/3/4)\nKey ... (0-65535)\nNetID ... (0-255)\nAddress ... (0-65535)\nAddress 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.";

const char PROGMEM ERROR_CHANNEL[] = "Must be 0-83";
const char PROGMEM ERROR_AIR_RATE[] = "Must be 300/1200/2400/4800/9600/19200/38400/62500";
const char PROGMEM ERROR_PACKET_SIZE[] = "Must be 32/64/128/240";
const char PROGMEM ERROR_POWER[] = "Must be 1/2/3/4";
const char PROGMEM ERROR_KEY[] = "Must be 0-65535";
const char PROGMEM ERROR_NET_ID[] = "Must be 0-255";
const char PROGMEM ERROR_ADDRESS[] = "Must be 0-65535, address 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.";

const char ADDRESS_CHANNEL[] = "CHANNEL";
const char ADDRESS_RATE[] = "RATE";
const char ADDRESS_PACKET[] = "PACKET";
const char ADDRESS_POWER[] = "POWER";
const char ADDRESS_KEY[] = "KEY";
const char ADDRESS_NET_ID[] = "NET_ID";
const char ADDRESS_ADDRESS[] = "ADDRESS";

const String BLUETOOTH_NAME = "ESP32MESSENGER";

// size 320x240, 10 x 20 symbols with text size 2
const uint8_t DISPLAY_STEP_LINE = 16;
const uint8_t DISPLAY_STEP_SYMBOL = 12;

const uint32_t UPDATE_DISPLAY_DELAY = 100;
const uint32_t UPDATE_SYSTEM_DELAY = 0;
const uint32_t TASK_SYSTEM_PROCESSES_STACK = 50000;
const uint32_t TASK_UPDATE_DISPLAY_STACK = 50000;

const uint32_t LORA_UPDATE_AMBIENT_DELAY = 5000; // If this happens frequently, the module will not work consistently, because the connection to the module is used to get the value.

bool loraIsBusy = false; // This value can be updated frequently as it checks the status of the pin, the connection to the module is not used

uint8_t loraCurrentMode = MODE_NORMAL;
uint8_t loraCurrentParityBit = PB_8N1;
uint8_t loraCurrentTransmissionMode = TXM_NORMAL;
uint8_t loraCurrentRepeaterMode = REPEATER_DISABLE;
uint8_t loraCurrentChannelMonitoringBeforeDataTransmissionMode = LBT_DISABLE; // In this mode, the transmitter waits until the channel becomes free before sending.
uint8_t loraCurrentPowerSavingModeWOR = WOR_TRANSMITTER; // In this mode, one-way data transmission from the transmitter to the receiver is carried out.
uint8_t loraCurrentPowerSavingModeWORCycle = WOR2000;
uint8_t loraCurrentAppendRSSItoTheEndOfReceivedDataMode = RSSI_ENABLE;
uint8_t loraCurrentListeningToTheAirAmbientMode = RSSI_ENABLE;
uint8_t loraCurrentChannel = 40; // 0-83
uint8_t loraCurrentAirDataRate = ADR_2400; // 300/1200/2400/4800/9600/19200/38400/62500
uint8_t loraCurrentPacketSize = PACKET240; // 32/64/128/240
uint8_t loraCurrentUartSpeed = UBR_9600; // 1200/2400/4800/9600/19200/38400/57600/115200 but stable at speed 9600, configuration only at speed 9600
uint16_t esp32currentUartSerialSpeed = 9600; // 1200/2400/4800/9600/19200/38400/57600/115200 but stable at speed 9600, configuration only at speed 9600
uint8_t loraCurrentPower = TP_MAX; // 1/2/3/4
uint16_t loraCurrentEncryptionKey = 0x0000; // 0-65535, must be the same for 2 modules
uint16_t loraCurrentAddress = 0xFFFF; // 0-65535, address 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.
uint8_t loraCurrentNetID = 0x00; // 0-255, must be the same for 2 modules
uint8_t memoryForSavingSettingsToTheModule = TEMPORARY; // TEMPORARY/PERMANENT

String textToSend = "";
String textLastSend = "";
String textReceived = "";
uint32_t timeCodeReceived = 0;
uint32_t timeCodeUpdateAmbient = 0;
uint8_t loraRssiLastReceive = 0;
uint8_t loraRssiAmbient = 0;

void setupLora() {
    Lora.setUARTBaudRate(loraCurrentUartSpeed);
    Lora.setMode(loraCurrentMode);
    Lora.setParityBit(loraCurrentParityBit);
    Lora.setTransmissionMode(loraCurrentTransmissionMode);
    Lora.setRepeater(loraCurrentRepeaterMode);
    Lora.setLBT(loraCurrentChannelMonitoringBeforeDataTransmissionMode);
    Lora.setWOR(loraCurrentPowerSavingModeWOR);
    Lora.setWORCycle(loraCurrentPowerSavingModeWORCycle);
    Lora.setRSSIInPacket(loraCurrentAppendRSSItoTheEndOfReceivedDataMode);
    Lora.setRSSIAmbient(loraCurrentListeningToTheAirAmbientMode);
    Lora.setTransmitPower(loraCurrentPower);
    Lora.setChannel(loraCurrentChannel);
    Lora.setAddress(loraCurrentAddress);
    Lora.setNetID(loraCurrentNetID);
    Lora.setAirDataRate(loraCurrentAirDataRate);
    Lora.setPacketLength(loraCurrentPacketSize);
    Lora.writeSettings(memoryForSavingSettingsToTheModule);
    Lora.writeCryptKey(loraCurrentEncryptionKey, memoryForSavingSettingsToTheModule);
//    Lora.writeSettingsWireless(TEMPORARY);
//    Lora.writeCryptKeyWireless(0x0128, TEMPORARY);
}

uint16_t getLine(uint8_t numberOfLine) {
    return numberOfLine * DISPLAY_STEP_LINE;
}

uint8_t getSymbol(uint8_t numberOfSymbol) {
    return numberOfSymbol * DISPLAY_STEP_SYMBOL;
}

void readDataFromEEPROM() {
    Preferences.begin("Preferences", false);
    uint16_t channel = Preferences.getUShort(ADDRESS_CHANNEL, 65535);
    if (channel != 65535) {
        loraCurrentChannel = channel;
    }
    uint16_t airDataRate = Preferences.getUShort(ADDRESS_RATE, 65535);
    if (airDataRate != 65535) {
        loraCurrentAirDataRate = airDataRate;
    }
    uint16_t packetSize = Preferences.getUShort(ADDRESS_PACKET, 65535);
    if (packetSize != 65535) {
        loraCurrentPacketSize = packetSize;
    }
    uint16_t power = Preferences.getUShort(ADDRESS_POWER, 65535);
    if (power != 65535) {
        loraCurrentPower = power;
    }
    uint32_t address = Preferences.getUInt(ADDRESS_POWER, 4294967295);
    if (address != 4294967295) {
        loraCurrentAddress = address;
    }
    uint32_t key = Preferences.getUInt(ADDRESS_KEY, 4294967295);
    if (key != 4294967295) {
        loraCurrentEncryptionKey = key;
    }
    uint16_t netID = Preferences.getUShort(ADDRESS_NET_ID, 65535);
    if (netID != 65535) {
        loraCurrentNetID = netID;
    }
}

void printText(String text, uint8_t line, uint8_t position, uint8_t clearSymbolsX, uint8_t clearSymbolsY) {
    Display.setCursor(getSymbol(position), getLine(line));
    Display.fillRect(
            getSymbol(position),
            getLine(line),
            getSymbol(clearSymbolsX),
            DISPLAY_STEP_LINE * clearSymbolsY,
            ST77XX_BLACK
    );
    Display.print(text);
}

void updateDisplay() {
    printText(String("Ambi:") + loraRssiAmbient, 0, 0, 10, 1);
    if (loraIsBusy) {
        Display.setTextColor(ST77XX_RED);
        printText("State:BUSY", 0, 10, 10, 1);
    } else {
        Display.setTextColor(ST77XX_GREEN);
        printText("State:FREE", 0, 10, 10, 1);
    }
    Display.setTextColor(ST77XX_WHITE);
    printText(String("RSSI:") + loraRssiLastReceive, 1, 0, 10, 1);
    printText(String("Chan: ") + loraCurrentChannel, 1, 10, 10, 1);
    switch (loraCurrentAirDataRate) {
        case ADR_300: {
            printText("Rate:300", 2, 0, 10, 1);
            break;
        }
        case ADR_1200: {
            printText("Rate:1200", 2, 0, 10, 1);
            break;
        }
        case ADR_2400: {
            printText("Rate:2400", 2, 0, 10, 1);
            break;
        }
        case ADR_4800: {
            printText("Rate:4800", 2, 0, 10, 1);
            break;
        }
        case ADR_9600: {
            printText("Rate:9600", 2, 0, 10, 1);
            break;
        }
        case ADR_19200: {
            printText("Rate:19200", 2, 0, 10, 1);
            break;
        }
        case ADR_38400: {
            printText("Rate:38400", 2, 0, 10, 1);
            break;
        }
        case ADR_62500: {
            printText("Rate:62500", 2, 0, 10, 1);
            break;
        }
    }
    switch (loraCurrentPacketSize) {
        case PACKET240: {
            printText("Pack: 240", 2, 10, 10, 1);
            break;
        }
        case PACKET128: {
            printText("Pack: 128", 2, 10, 10, 1);
            break;
        }
        case PACKET64: {
            printText("Pack: 64", 2, 10, 10, 1);
            break;
        }
        case PACKET32: {
            printText("Pack: 32", 2, 10, 10, 1);
            break;
        }
    }
    char keyHex[10] = "";
    printText(String("Key: ") + ltoa(loraCurrentEncryptionKey, keyHex, 16), 3, 0, 10, 1);
    switch (loraCurrentPower) {
        case TP_MAX: {
            printText("Power:Max", 3, 10, 10, 1);
            break;
        }
        case TP_HIGH: {
            printText("Power:High", 3, 10, 10, 1);
            break;
        }
        case TP_MID: {
            printText("Power:Mid", 3, 10, 10, 1);
            break;
        }
        case TP_LOW: {
            printText("Power:Low", 3, 10, 10, 1);
            break;
        }
    }
    char addressHex[10] = "";
    printText(String("Addr:") + ltoa(loraCurrentAddress, addressHex, 16), 4, 0, 10, 1);
    char netIDHex[10] = "";
    printText(String("NetID:") + ltoa(loraCurrentNetID, netIDHex, 16), 4, 10, 10, 1);
    printText(String(timeCodeReceived), 5, 0, 20, 1);
    printText(textReceived, 6, 0, 20, 14);
}



// commands will be json for mobile client
void parseCommand(String buffer) {
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SEND)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SEND, "");
        if (textToSend.isEmpty()) {
            textToSend = buffer;
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println("Message queued for sending:");
                SerialBluetooth.println(textToSend);
            }
        } else {
            textToSend = textToSend + "/n" + buffer;
        }
        textLastSend = textToSend;
        return;
    }
    if (buffer.equals((__FlashStringHelper *) COMMAND_RESET)) {
        Preferences.clear();
        loraCurrentChannel = 40;
        loraCurrentAirDataRate = ADR_300;
        loraCurrentPacketSize = PACKET32;
        loraCurrentPower = TP_MAX;
        loraCurrentEncryptionKey = 128;
        loraCurrentAddress = 0;
        loraCurrentNetID = 0;
        setupLora();
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println((__FlashStringHelper *) READY);
        }
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_NET_ID)) {
        buffer.replace((__FlashStringHelper *) COMMAND_NET_ID, "");
        uint8_t netID = buffer.toInt();
        if (netID >= 0 && netID <= 255) {
            loraCurrentNetID = netID;
            Lora.setNetID(netID);
            Lora.writeSettings(TEMPORARY);
            Preferences.putUShort(ADDRESS_NET_ID, netID);
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) READY);
            }
        } else {
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) ERROR_NET_ID);
            }
        }
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_ADDRESS)) {
        buffer.replace((__FlashStringHelper *) COMMAND_ADDRESS, "");
        uint16_t address = buffer.toInt();
        if (address >= 0 && address <= 65535) {
            loraCurrentAddress = address;
            Lora.setAddress(address);
            Lora.writeSettings(TEMPORARY);
            Preferences.putUShort(ADDRESS_ADDRESS, address);
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) READY);
            }
        } else {
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) ERROR_ADDRESS);
            }
        }
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_KEY)) {
        buffer.replace((__FlashStringHelper *) COMMAND_KEY, "");
        uint16_t key = buffer.toInt();
        if (key >= 0 && key <= 65535) {
            loraCurrentEncryptionKey = key;
            Lora.writeCryptKey(key, TEMPORARY);
            Preferences.putUInt(ADDRESS_KEY, key);
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) READY);
            }
        } else {
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) ERROR_KEY);
            }
        }
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_POWER)) {
        buffer.replace((__FlashStringHelper *) COMMAND_POWER, "");
        uint8_t power = buffer.toInt();
        switch (power) {
            case 1: {
                loraCurrentPower = TP_LOW;
                Lora.setTransmitPower(TP_LOW);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_POWER, TP_LOW);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 2: {
                loraCurrentPower = TP_MID;
                Lora.setTransmitPower(TP_MID);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_POWER, TP_MID);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 3: {
                loraCurrentPower = TP_HIGH;
                Lora.setTransmitPower(TP_HIGH);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_POWER, TP_HIGH);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 4: {
                loraCurrentPower = TP_MAX;
                Lora.setTransmitPower(TP_MAX);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_POWER, TP_MAX);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            default: {
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) ERROR_POWER);
                }
                return;
            }
        }
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_PACKET)) {
        buffer.replace((__FlashStringHelper *) COMMAND_PACKET, "");
        uint8_t packet = buffer.toInt();
        switch (packet) {
            case 32: {
                loraCurrentPacketSize = PACKET32;
                Lora.setPacketLength(PACKET32);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_PACKET, PACKET32);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 64: {
                loraCurrentPacketSize = PACKET64;
                Lora.setPacketLength(PACKET64);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_PACKET, PACKET64);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 128: {
                loraCurrentPacketSize = PACKET128;
                Lora.setPacketLength(PACKET128);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_PACKET, PACKET128);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 240: {
                loraCurrentPacketSize = PACKET240;
                Lora.setPacketLength(PACKET240);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_PACKET, PACKET240);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            default: {
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) ERROR_PACKET_SIZE);
                }
                return;
            }
        }
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_CHANNEL)) {
        buffer.replace((__FlashStringHelper *) COMMAND_CHANNEL, "");
        uint8_t channel = buffer.toInt();
        if (channel >= 0 && channel <= 83) {
            loraCurrentChannel = channel;
            Lora.setChannel(channel);
            Lora.writeSettings(TEMPORARY);
            Preferences.putUShort(ADDRESS_CHANNEL, channel);
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) READY);
            }
        } else {
            if (SerialBluetooth.connected()) {
                SerialBluetooth.println((__FlashStringHelper *) ERROR_CHANNEL);
            }
        }
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_RATE)) {
        buffer.replace((__FlashStringHelper *) COMMAND_RATE, "");
        int rate = buffer.toInt();
        switch (rate) {
            case 300: {
                loraCurrentAirDataRate = ADR_300;
                Lora.setAirDataRate(ADR_300);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_300);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 1200: {
                loraCurrentAirDataRate = ADR_1200;
                Lora.setAirDataRate(ADR_1200);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_1200);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 2400: {
                loraCurrentAirDataRate = ADR_2400;
                Lora.setAirDataRate(ADR_2400);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_2400);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 4800: {
                loraCurrentAirDataRate = ADR_4800;
                Lora.setAirDataRate(ADR_4800);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_4800);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 9600: {
                loraCurrentAirDataRate = ADR_9600;
                Lora.setAirDataRate(ADR_9600);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_9600);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 19200: {
                loraCurrentAirDataRate = ADR_19200;
                Lora.setAirDataRate(ADR_19200);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_19200);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 38400: {
                loraCurrentAirDataRate = ADR_38400;
                Lora.setAirDataRate(ADR_38400);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_38400);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 62500: {
                loraCurrentAirDataRate = ADR_62500;
                Lora.setAirDataRate(ADR_62500);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_62500);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                    return;
                }
            }
            default: {
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) ERROR_AIR_RATE);
                }
                return;
            }
        }
    }
    if (SerialBluetooth.connected()) {
        SerialBluetooth.println((__FlashStringHelper *) COMMANDS_ALL);
        return;
    }
}


void updateSystem() {
    loraIsBusy = Lora.getBusy();
    uint32_t currentTime = millis();
    if (!loraIsBusy && currentTime > timeCodeUpdateAmbient + LORA_UPDATE_AMBIENT_DELAY) {
        loraRssiAmbient = Lora.getRSSI(RSSI_AMBIENT);
        timeCodeUpdateAmbient = currentTime;
    }
    if (Lora.available() > 0) {
        textReceived = Serial1.readString();
        loraRssiLastReceive = Lora.getRSSI(RSSI_LAST_RECEIVE);
        timeCodeReceived = millis();
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println();
            SerialBluetooth.println("Message received:");
            SerialBluetooth.println(textReceived);
            SerialBluetooth.print("RSSI received: ");
            SerialBluetooth.println(loraRssiLastReceive);
            SerialBluetooth.print("RSSI ambient: ");
            SerialBluetooth.println(loraRssiAmbient);
            SerialBluetooth.println();
        }
    }
    if (SerialBluetooth.connected() && SerialBluetooth.available()) {
        parseCommand(SerialBluetooth.readString());
    }
    if (!textToSend.isEmpty() && Serial1.available()) {
        Serial1.print(textToSend);
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println("The message was sent and most likely received by another device:");
            SerialBluetooth.println(textToSend);
        }
        textToSend = "";
    }
}

void taskSystemProcesses(void *pvParameters) {
    for (;;) {
        updateSystem();
        if (UPDATE_SYSTEM_DELAY > 0)
            delay(UPDATE_SYSTEM_DELAY);
    }
}

void taskUpdateDisplay(void *pvParameters) {
    for (;;) {
        updateDisplay();
        if (UPDATE_DISPLAY_DELAY > 0)
            delay(UPDATE_DISPLAY_DELAY);
    }
}

void setupTasksForESP32Cores() {
    xTaskCreatePinnedToCore(
            taskUpdateDisplay,
            "taskUpdateDisplay",
            TASK_UPDATE_DISPLAY_STACK,
            NULL,
            1,
            &TaskUpdateDisplay,
            0);
    xTaskCreatePinnedToCore(
            taskSystemProcesses,
            "taskSystemProcesses",
            TASK_SYSTEM_PROCESSES_STACK,
            NULL,
            1,
            &TaskSystemProcesses,
            1);
}

void setupDisplay() {
    Display.init(240, 320);
    Display.setSPISpeed(40000000);
    Display.setTextColor(ST77XX_WHITE);
    Display.setRotation(2);
    Display.setTextSize(2);
    Display.setTextWrap(true);
    Display.fillScreen(ST77XX_BLACK);
    Display.setCursor(0, 0);
    Display.println("Start");
}

void setup() {
    SerialBluetooth.begin(BLUETOOTH_NAME);
    Serial1.begin(esp32currentUartSerialSpeed, SERIAL_8N1, PIN_RX, PIN_TX);

    readDataFromEEPROM();
    setupDisplay();
    Lora.init();
    setupLora();

    setupTasksForESP32Cores();
}

void loop() {
}