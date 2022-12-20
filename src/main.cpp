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

const char PROGMEM MESSAGE_READY[] = "Ready";
const char PROGMEM MESSAGE_ERROR[] = "Error";
const char PROGMEM MESSAGE_SERIAL_IN_PROGRAM_MODE[] = "Setup serial port in program mode";
const char PROGMEM MESSAGE_SERIAL_IN_WORK_MODE[] = "Setup serial port in work mode";

const char PROGMEM COMMAND_RESET_LORA_SETTINGS[] = "ResetSettings";
const char PROGMEM COMMAND_GET_LORA_SETTINGS[] = "GetSettings";
const char PROGMEM COMMAND_PRINT_COMMANDS[] = "PrintCommands";
const char PROGMEM COMMAND_FREE_PREFERENCES_ENTRIES[] = "PrintFreePreferencesEntries";

const char PROGMEM COMMAND_SET_LORA_CHANNEL[] = "SetChannel:";
const char PROGMEM COMMAND_SET_LORA_AIR_SPEED[] = "SetAirSpeed:";
const char PROGMEM COMMAND_SET_LORA_SERIAL_SPEED[] = "SetSerialSpeed:";
const char PROGMEM COMMAND_SET_LORA_PACKET_SIZE[] = "SetPacketSize:";
const char PROGMEM COMMAND_SET_LORA_POWER[] = "SetPower:";
const char PROGMEM COMMAND_SET_LORA_KEY[] = "SetKey:";
const char PROGMEM COMMAND_SET_LORA_NET_ID[] = "SetNetID:";
const char PROGMEM COMMAND_SET_LORA_ADDRESS[] = "SetAddress:";

const char PROGMEM COMMAND_GET_LORA_CHANNEL[] = "GetChannel";
const char PROGMEM COMMAND_GET_LORA_AIR_SPEED[] = "GetAirSpeed";
const char PROGMEM COMMAND_GET_LORA_SERIAL_SPEED[] = "GetSerialSpeed";
const char PROGMEM COMMAND_GET_BOARD_SERIAL_SPEED[] = "GetBoardSerialSpeed";
const char PROGMEM COMMAND_GET_LORA_PACKET_SIZE[] = "GetPacketSize";
const char PROGMEM COMMAND_GET_LORA_POWER[] = "GetPower";
const char PROGMEM COMMAND_GET_LORA_KEY[] = "GetKey";
const char PROGMEM COMMAND_GET_LORA_NET_ID[] = "GetNetID";
const char PROGMEM COMMAND_GET_LORA_ADDRESS[] = "GetAddress";
const char PROGMEM COMMAND_GET_LORA_MODE[] = "GetMode";
const char PROGMEM COMMAND_GET_LORA_PARITY_BIT[] = "GetParityBit";
const char PROGMEM COMMAND_GET_LORA_TRANSMISSION_MODE[] = "GetTransmissionMode";
const char PROGMEM COMMAND_GET_LORA_REPEATER_MODE[] = "GetRepeaterMode";
const char PROGMEM COMMAND_GET_LORA_MONITORING_BEFORE_DATA_TRANSMISSION_MODE[] = "GetMonitoringBeforeDataTransmissionMode";
const char PROGMEM COMMAND_GET_LORA_WOR_MODE[] = "GetWORMode";
const char PROGMEM COMMAND_GET_LORA_WOR_CYCLE[] = "GetWORCycle";
const char PROGMEM COMMAND_GET_LORA_RSSI_TO_THE_END_OF_RECEIVED_DATA_MODE[] = "GetRSSItoTheEndOfReceivedDataMode";
const char PROGMEM COMMAND_GET_LORA_LISTENING_TO_AIR_AMBIENT_MODE[] = "GetListeningToAirAmbientMode";
const char PROGMEM COMMAND_GET_LORA_SAVING_SETTINGS_MEMORY[] = "GetSavingSettingsMemory";
const char PROGMEM COMMAND_GET_BOARD_FREE_RAM_MEMORY[] = "GetFreeRamMemory";

const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_CHANNEL[] = "GetChannel:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_AIR_SPEED[] = "GetAirSpeed:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_SERIAL_SPEED[] = "GetSerialSpeed:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_BOARD_SERIAL_SPEED[] = "GetBoardSerialSpeed:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_PACKET_SIZE[] = "GetPacketSize:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_POWER[] = "GetPower:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_KEY[] = "GetKey:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_NET_ID[] = "GetNetID:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_ADDRESS[] = "GetAddress:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_MODE[] = "GetMode:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_PARITY_BIT[] = "GetParityBit:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_TRANSMISSION_MODE[] = "GetTransmissionMode:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_REPEATER_MODE[] = "GetRepeaterMode:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_MONITORING_BEFORE_DATA_TRANSMISSION_MODE[] = "GetMonitoringBeforeDataTransmissionMode:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_WOR_MODE[] = "GetWORMode:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_WOR_CYCLE[] = "GetWORCycle:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_RSSI_TO_THE_END_OF_RECEIVED_DATA_MODE[] = "GetRSSItoTheEndOfReceivedDataMode:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_LISTENING_TO_AIR_AMBIENT_MODE[] = "GetListeningToAirAmbientMode:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_LORA_SAVING_SETTINGS_MEMORY[] = "GetSavingSettingsMemory:";
const char PROGMEM COMMAND_GET_RETURN_VALUE_BOARD_FREE_RAM_MEMORY[] = "GetFreeRamMemory:";

const char PROGMEM MESSAGE_LORA_RSSI_AMBIENT[] = "RSSI ambient: ";
const char PROGMEM MESSAGE_LORA_MESSAGE_RECEIVED[] = "Message received: ";
const char PROGMEM MESSAGE_LORA_RSSI_FOR_LAST_MESSAGE_RECEIVED[] = "RSSI received: ";
const char PROGMEM MESSAGE_LORA_MESSAGE_QUEUED_FOR_SENDING[] = "Message queued for sending: ";

const char PROGMEM MESSAGE_COMMANDS_ALL[] = "Commands:\n...(some text) (will be sended)\nSetChannel:...(E22-400T30D: 0-83, Frequency 410.125 + CH*1M E22-900T30D: 0-80, Frequency 850.125 + CH*1M E22-230T30D: 0-83, Frequency 220.125 + CH*0.25M)\nSetSpeed:...(300/1200/2400/4800/9600/19200/38400/62500)\nSetPacketSize:...(32/64/128/240)\nSetPower:...(1/2/3/4, E22-...T30D: 21dbm/24dbm/27dbm/30dbm, E22-...T22D: 10dbm/13dbm/17dbm/22dbm)\nSetKey:...(0-65535)\nNetID ... (0-255)\nSetAddress:...(0-65535)\nAddress 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.\nResetSettings";

const char PROGMEM MESSAGE_ERROR_CHANNEL[] = "Must be: E22-400T30D: 0-83, Frequency 410.125 + CH*1M E22-900T30D: 0-80, Frequency 850.125 + CH*1M E22-230T30D: 0-83, Frequency 220.125 + CH*0.25M";
const char PROGMEM MESSAGE_ERROR_AIR_SPEED[] = "Must be 300/1200/2400/4800/9600/19200/38400/62500";
const char PROGMEM MESSAGE_ERROR_PACKET_SIZE[] = "Must be 32/64/128/240";
const char PROGMEM MESSAGE_ERROR_POWER[] = "Must be 1/2/3/4";
const char PROGMEM MESSAGE_ERROR_KEY[] = "Must be 0-65535";
const char PROGMEM MESSAGE_ERROR_NET_ID[] = "Must be 0-255";
const char PROGMEM MESSAGE_ERROR_SERIAL_SPEED[] = "Must be 1200/2400/4800/9600/19200/38400/57600/115200";
const char PROGMEM MESSAGE_ERROR_ADDRESS[] = "Must be 0-65535, address 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.";

const char PROGMEM KEY_LORA_CHANNEL[] = "channel";
const char PROGMEM KEY_LORA_AIR_SPEED[] = "rate";
const char PROGMEM KEY_LORA_PACKET_SIZE[] = "packet";
const char PROGMEM KEY_LORA_POWER[] = "power";
const char PROGMEM KEY_LORA_KEY[] = "key";
const char PROGMEM KEY_LORA_NET_ID[] = "netid";
const char PROGMEM KEY_LORA_SERIAL[] = "serial";

const uint8_t maxKeySize = 24;

const char PROGMEM KEY_LORA_ADDRESS[] = "address";

const char KEY_PREFERENCES[] = "preferences";

const String BLUETOOTH_NAME = "ESP32MESSENGER";

// size 320x240, 10 x 20 symbols with text size 2
const uint8_t DISPLAY_STEP_LINE = 16;
const uint8_t DISPLAY_STEP_SYMBOL = 12;

const uint32_t UPDATE_DISPLAY_DELAY = 100;
const uint32_t UPDATE_SYSTEM_DELAY = 0;
const uint32_t TASK_SYSTEM_PROCESSES_STACK = 50000;
const uint32_t TASK_UPDATE_DISPLAY_STACK = 50000;
const uint32_t defaultPreferencesValue = 4294967295;
const uint32_t uartSpeedInSetupMode = 9600;

// If this happens frequently, the module will not work consistently, because the connection to the module is used to get the value.
const uint32_t LORA_UPDATE_AMBIENT_DELAY = 5000;

// This value can be updated frequently as it checks the status of the pin, the connection to the module is not used
bool loraIsBusy = false;

uint8_t loraCurrentModeConst = MODE_NORMAL;
uint8_t loraCurrentParityBitConst = PB_8N1;
uint8_t loraCurrentTransmissionModeConst = TXM_NORMAL;
uint8_t loraCurrentRepeaterModeConst = REPEATER_DISABLE;
// In this mode, the transmitter waits until the channel becomes free before sending.
uint8_t loraCurrentChannelMonitoringBeforeDataTransmissionModeConst = LBT_DISABLE;
// In this mode, one-way data transmission from the transmitter to the receiver is carried out.
uint8_t loraCurrentPowerSavingWORModeConst = WOR_TRANSMITTER;
uint8_t loraCurrentPowerSavingWORCycleConst = WOR2000;
uint8_t loraCurrentAppendRSSItoTheEndOfReceivedDataModeConst = RSSI_ENABLE;
uint8_t loraCurrentListeningToTheAirAmbientModeConst = RSSI_ENABLE;
// Channels:
// E22-400T30D: 0-83, Frequency= 410.125 + CH*1M
// E22-900T30D: 0-80, Frequency= 850.125 + CH*1M
// E22-230T30D: 0-83, Frequency = 220.125 + CH*0.25M
uint8_t loraCurrentChannelIndex = 40;
// 300/1200/2400/4800/9600/19200/38400/62500, 62500 = 62.5kbs
uint8_t loraCurrentAirSpeedConst = ADR_2400;
// 32/64/128/240, packet size in bytes
uint8_t loraCurrentPacketSizeConst = PACKET240;
// 1200/2400/4800/9600/19200/38400/57600/115200 but stable at speed 9600, configuration only at speed 9600
uint8_t loraCurrentSerialSpeedConst = UBR_9600;
// 1200/2400/4800/9600/19200/38400/57600/115200 but stable at speed 9600, configuration only at speed 9600
uint32_t esp32currentSerialSpeed = 9600;
// 1/2/3/4, E22-...T30D: 21dbm 24dbm 27dbm 30dbm, E22-...T22D: 10dbm 13dbm 17dbm 22dbm
uint8_t loraCurrentPowerConst = TP_MAX;
// 0-65535, must be the same for 2 modules
uint16_t loraCurrentEncryptionKeyIndex = 0x0000;
// 0-65535, address 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.
uint16_t loraCurrentAddressIndex = 0xFFFF;
// 0-255, must be the same for 2 modules
uint8_t loraCurrentNetIDIndex = 0x00;
// TEMPORARY/PERMANENT memory
uint8_t memoryForSavingSettingsToTheModuleConst = TEMPORARY;

String textToSend = "";
String textLastSend = "";
String textReceived = "";
uint32_t timeCodeReceived = 0;
uint32_t timeCodeUpdateAmbient = 0;
uint8_t loraRssiLastReceive = 0;
uint8_t loraRssiAmbient = 0;

void setupSerialInProgramMode() {
    if (!Serial1.available()) {
        Serial1.begin(uartSpeedInSetupMode, SERIAL_8N1, PIN_RX, PIN_TX);
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println((__FlashStringHelper *) MESSAGE_SERIAL_IN_PROGRAM_MODE);
        }
        return;
    }
    if (Serial1.baudRate() != uartSpeedInSetupMode) {
        Serial1.end();
        Serial1.begin(uartSpeedInSetupMode, SERIAL_8N1, PIN_RX, PIN_TX);
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println((__FlashStringHelper *) MESSAGE_SERIAL_IN_PROGRAM_MODE);
        }
    }
}

void setupSerialInWorkMode() {
    if (!Serial1.available()) {
        Serial1.begin(esp32currentSerialSpeed, SERIAL_8N1, PIN_RX, PIN_TX);
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println((__FlashStringHelper *) MESSAGE_SERIAL_IN_WORK_MODE);
        }
        return;
    }
    if (Serial1.baudRate() != esp32currentSerialSpeed) {
        Serial1.end();
        Serial1.begin(esp32currentSerialSpeed, SERIAL_8N1, PIN_RX, PIN_TX);
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println((__FlashStringHelper *) MESSAGE_SERIAL_IN_WORK_MODE);
        }
    }
}

void setupLora() {
    setupSerialInProgramMode();
    Lora.setUARTBaudRate(loraCurrentSerialSpeedConst);
    Lora.setMode(loraCurrentModeConst);
    Lora.setParityBit(loraCurrentParityBitConst);
    Lora.setTransmissionMode(loraCurrentTransmissionModeConst);
    Lora.setRepeater(loraCurrentRepeaterModeConst);
    Lora.setLBT(loraCurrentChannelMonitoringBeforeDataTransmissionModeConst);
    Lora.setWOR(loraCurrentPowerSavingWORModeConst);
    Lora.setWORCycle(loraCurrentPowerSavingWORCycleConst);
    Lora.setRSSIInPacket(loraCurrentAppendRSSItoTheEndOfReceivedDataModeConst);
    Lora.setRSSIAmbient(loraCurrentListeningToTheAirAmbientModeConst);
    Lora.setTransmitPower(loraCurrentPowerConst);
    Lora.setChannel(loraCurrentChannelIndex);
    Lora.setAddress(loraCurrentAddressIndex);
    Lora.setNetID(loraCurrentNetIDIndex);
    Lora.setAirDataRate(loraCurrentAirSpeedConst);
    Lora.setPacketLength(loraCurrentPacketSizeConst);
    Lora.writeSettings(memoryForSavingSettingsToTheModuleConst);
    Lora.writeCryptKey(loraCurrentEncryptionKeyIndex, memoryForSavingSettingsToTheModuleConst);
//    Lora.writeSettingsWireless(TEMPORARY);
//    Lora.writeCryptKeyWireless(0x0128, TEMPORARY);
    setupSerialInWorkMode();
}

uint16_t getDisplayLineCoordY(uint8_t numberOfLine) {
    return numberOfLine * DISPLAY_STEP_LINE;
}

uint8_t getDisplaySymbolCoordX(uint8_t numberOfSymbol) {
    return numberOfSymbol * DISPLAY_STEP_SYMBOL;
}

uint32_t getLoraSerialSpeedIndexByConst(uint8_t serialSpeedConst) {
    switch (serialSpeedConst) {
        case UBR_1200:
            return 1200;
        case UBR_2400:
            return 2400;
        case UBR_4800:
            return 4800;
        case UBR_9600:
            return 9600;
        case UBR_19200:
            return 19200;
        case UBR_38400:
            return 38400;
        case UBR_57600:
            return 57600;
        case UBR_115200:
            return 115200;
        default:
            return 0;
    }
}

uint8_t getLoraSerialSpeedConstByIndex(uint32_t serialSpeedIndex) {
    switch (serialSpeedIndex) {
        case 1200:
            return UBR_1200;
        case 2400:
            return UBR_2400;
        case 4800:
            return UBR_4800;
        case 9600:
            return UBR_9600;
        case 19200:
            return UBR_19200;
        case 38400:
            return UBR_38400;
        case 57600:
            return UBR_57600;
        case 115200:
            return UBR_115200;
        default:
            return 0;
    }
}

uint16_t getLoraAirSpeedIndexByConst(uint8_t airSpeedConst) {
    switch (airSpeedConst) {
        case ADR_300:
            return 300;
        case ADR_1200:
            return 1200;
        case ADR_2400:
            return 2400;
        case ADR_4800:
            return 4800;
        case ADR_9600:
            return 9600;
        case ADR_19200:
            return 19200;
        case ADR_38400:
            return 38400;
        case ADR_62500:
            return 62500;
        default:
            return 0;
    }
}

uint8_t getLoraAirSpeedConstByIndex(uint16_t airSpeedIndex) {
    switch (airSpeedIndex) {
        case 300:
            return ADR_300;
        case 1200:
            return ADR_1200;
        case 2400:
            return ADR_2400;
        case 4800:
            return ADR_4800;
        case 9600:
            return ADR_9600;
        case 19200:
            return ADR_19200;
        case 38400:
            return ADR_38400;
        case 62500:
            return ADR_62500;
        default:
            return 0;
    }
}

uint8_t getLoraPacketSizeIndexByConst(uint8_t packetSizeConst) {
    switch (packetSizeConst) {
        case PACKET240:
            return 240;
        case PACKET128:
            return 128;
        case PACKET64:
            return 64;
        case PACKET32:
            return 32;
        default:
            return 0;
    }
}

uint8_t getLoraPacketSizeConstByIndex(uint8_t packetSizeIndex) {
    switch (packetSizeIndex) {
        case 32:
            return PACKET32;
        case 64:
            return PACKET64;
        case 128:
            return PACKET128;
        case 240:
            return PACKET240;
        default:
            return 0;
    }
}

uint8_t getPowerIndexByConst(uint8_t powerConst) {
    switch (powerConst) {
        case TP_MAX:
            return 4;
        case TP_HIGH:
            return 3;
        case TP_MID:
            return 2;
        case TP_LOW:
            return 1;
        default:
            return 0;
    }
}

uint8_t getPowerConstByIndex(uint8_t powerIndex) {
    switch (powerIndex) {
        case 1:
            return TP_LOW;
        case 2:
            return TP_MID;
        case 3:
            return TP_HIGH;
        case 4:
            return TP_MAX;
        default:
            return 0;
    }
}

void readDataFromEEPROM() {
    Preferences.begin(KEY_PREFERENCES, true);
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_CHANNEL;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    uint32_t currentValue = Preferences.getULong(preferencesKeyChar, defaultPreferencesValue);
    if (currentValue != defaultPreferencesValue) {
        loraCurrentChannelIndex = currentValue;
    }
    preferencesKeyString = (__FlashStringHelper *) KEY_LORA_AIR_SPEED;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    currentValue = Preferences.getULong(preferencesKeyChar, defaultPreferencesValue);
    if (currentValue != defaultPreferencesValue) {
        loraCurrentAirSpeedConst = currentValue;
    }
    preferencesKeyString = (__FlashStringHelper *) KEY_LORA_PACKET_SIZE;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    currentValue = Preferences.getULong(preferencesKeyChar, defaultPreferencesValue);
    if (currentValue != defaultPreferencesValue) {
        loraCurrentPacketSizeConst = currentValue;
    }
    preferencesKeyString = (__FlashStringHelper *) KEY_LORA_POWER;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    currentValue = Preferences.getULong(preferencesKeyChar, defaultPreferencesValue);
    if (currentValue != defaultPreferencesValue) {
        loraCurrentPowerConst = currentValue;
    }
    preferencesKeyString = (__FlashStringHelper *) KEY_LORA_POWER;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    currentValue = Preferences.getULong(preferencesKeyChar, defaultPreferencesValue);
    if (currentValue != defaultPreferencesValue) {
        loraCurrentAddressIndex = currentValue;
    }
    preferencesKeyString = (__FlashStringHelper *) KEY_LORA_KEY;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    currentValue = Preferences.getULong(preferencesKeyChar, defaultPreferencesValue);
    if (currentValue != defaultPreferencesValue) {
        loraCurrentEncryptionKeyIndex = currentValue;
    }
    preferencesKeyString = (__FlashStringHelper *) KEY_LORA_NET_ID;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    currentValue = Preferences.getULong(preferencesKeyChar, defaultPreferencesValue);
    if (currentValue != defaultPreferencesValue) {
        loraCurrentNetIDIndex = currentValue;
    }
    Preferences.end();
}

void printText(String text, uint8_t line, uint8_t position, uint8_t clearSymbolsX, uint8_t clearSymbolsY) {
    Display.setCursor(getDisplaySymbolCoordX(position), getDisplayLineCoordY(line));
    Display.fillRect(
            getDisplaySymbolCoordX(position),
            getDisplayLineCoordY(line),
            getDisplaySymbolCoordX(clearSymbolsX),
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
    printText(String("Chan: ") + loraCurrentChannelIndex, 1, 10, 10, 1);
    switch (loraCurrentAirSpeedConst) {
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
    switch (loraCurrentPacketSizeConst) {
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
    printText(String("Key: ") + ltoa(loraCurrentEncryptionKeyIndex, keyHex, 16), 3, 0, 10, 1);
    switch (loraCurrentPowerConst) {
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
    printText(String("Addr:") + ltoa(loraCurrentAddressIndex, addressHex, 16), 4, 0, 10, 1);
    char netIDHex[10] = "";
    printText(String("NetID:") + ltoa(loraCurrentNetIDIndex, netIDHex, 16), 4, 10, 10, 1);
    printText(String(timeCodeReceived), 5, 0, 20, 1);
    printText(textReceived, 6, 0, 20, 14);
}

void printAllCommandsToBluetoothSerial() {
    if (SerialBluetooth.connected()) {
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_COMMANDS_ALL);
    }
}

void printFreePreferencesEntries() {
    if (SerialBluetooth.connected()) {
        Preferences.begin(KEY_PREFERENCES, true);
        SerialBluetooth.println(Preferences.freeEntries());
        Preferences.end();
    }
}

void resetLoraSettings() {
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.clear();
    Preferences.end();
    loraCurrentModeConst = MODE_NORMAL;
    loraCurrentParityBitConst = PB_8N1;
    loraCurrentTransmissionModeConst = TXM_NORMAL;
    loraCurrentRepeaterModeConst = REPEATER_DISABLE;
    loraCurrentChannelMonitoringBeforeDataTransmissionModeConst = LBT_DISABLE;
    loraCurrentPowerSavingWORModeConst = WOR_TRANSMITTER;
    loraCurrentPowerSavingWORCycleConst = WOR2000;
    loraCurrentAppendRSSItoTheEndOfReceivedDataModeConst = RSSI_ENABLE;
    loraCurrentListeningToTheAirAmbientModeConst = RSSI_ENABLE;
    loraCurrentChannelIndex = 40;
    loraCurrentAirSpeedConst = ADR_2400;
    loraCurrentPacketSizeConst = PACKET240;
    loraCurrentSerialSpeedConst = UBR_9600;
    esp32currentSerialSpeed = 9600;
    loraCurrentPowerConst = TP_MAX;
    loraCurrentEncryptionKeyIndex = 0x0000;
    loraCurrentAddressIndex = 0xFFFF;
    loraCurrentNetIDIndex = 0x00;
    memoryForSavingSettingsToTheModuleConst = TEMPORARY;
    setupLora();
    if (SerialBluetooth.connected()) {
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_READY);
    }
}

void printCurrentLoraChannel() {
    if (SerialBluetooth.connected()) {
        loraCurrentChannelIndex = Lora.getChannel();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_CHANNEL;
        textBuffer = textBuffer + loraCurrentChannelIndex;
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentLoraAirSpeed() {
    if (SerialBluetooth.connected()) {
        loraCurrentAirSpeedConst = Lora.getAirDataRate();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_AIR_SPEED;
        textBuffer = textBuffer + getLoraAirSpeedIndexByConst(loraCurrentAirSpeedConst);
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentLoraPacketSize() {
    if (SerialBluetooth.connected()) {
        loraCurrentPacketSizeConst = Lora.getPacketLength();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_PACKET_SIZE;
        textBuffer = textBuffer + getLoraPacketSizeIndexByConst(loraCurrentPacketSizeConst);
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentLoraPower() {
    if (SerialBluetooth.connected()) {
        loraCurrentPowerConst = Lora.getTransmitPower();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_POWER;
        textBuffer = textBuffer + getPowerIndexByConst(loraCurrentPowerConst);
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentLoraSerialSpeed() {
    if (SerialBluetooth.connected()) {
        loraCurrentSerialSpeedConst = Lora.getUARTBaudRate();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_SERIAL_SPEED;
        textBuffer = textBuffer + getLoraSerialSpeedIndexByConst(loraCurrentSerialSpeedConst);
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentBoardSerialSpeed() {
    if (SerialBluetooth.connected()) {
        esp32currentSerialSpeed = Serial1.baudRate();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_BOARD_SERIAL_SPEED;
        textBuffer = textBuffer + esp32currentSerialSpeed;
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentLoraKey() {
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_KEY;
        textBuffer = textBuffer + loraCurrentEncryptionKeyIndex;
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentLoraNetID() {
    if (SerialBluetooth.connected()) {
        loraCurrentNetIDIndex = Lora.getNetID();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_NET_ID;
        textBuffer = textBuffer + loraCurrentAddressIndex;
        SerialBluetooth.println(textBuffer);
    }
}

void printCurrentLoraAddress() {
    if (SerialBluetooth.connected()) {
        loraCurrentAddressIndex = Lora.getAddress();
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_ADDRESS;
        textBuffer = textBuffer + loraCurrentAddressIndex;
        SerialBluetooth.println(textBuffer);
    }
}

void printAllLoraSettingsToBluetoothSerial() {
    printCurrentLoraChannel();
    printCurrentLoraAirSpeed();
    printCurrentLoraPacketSize();
    printCurrentLoraPower();
    printCurrentLoraKey();
    printCurrentLoraNetID();
    printCurrentLoraAddress();
}

void setupLoraPacketSize(uint8_t packetSizeIndex) {
    uint8_t packetSizeConst = getLoraPacketSizeConstByIndex(packetSizeIndex);
    if (packetSizeConst == 0) {
        if (SerialBluetooth.connected()) SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_PACKET_SIZE);
        return;
    }
    setupSerialInProgramMode();
    Lora.setPacketLength(packetSizeConst);
    Lora.writeSettings(TEMPORARY);
    setupSerialInWorkMode();
    loraCurrentPacketSizeConst = Lora.getPacketLength();
    if (loraCurrentPacketSizeConst != packetSizeConst) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
        return;
    }
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_PACKET_SIZE;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.putULong(preferencesKeyChar, loraCurrentPacketSizeConst);
    uint8_t saved = Preferences.getULong(preferencesKeyChar, 0);
    if (saved != packetSizeConst && SerialBluetooth.connected())
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
    Preferences.end();
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_PACKET_SIZE;
        textBuffer = textBuffer + getLoraPacketSizeIndexByConst(loraCurrentPacketSizeConst);
        SerialBluetooth.println(textBuffer);
    }
}


void setupLoraTransmitPower(uint8_t powerIndex) {
    uint8_t powerConst = getPowerConstByIndex(powerIndex);
    if (powerConst == 0) {
        if (SerialBluetooth.connected()) SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_POWER);
        return;
    }
    setupSerialInProgramMode();
    Lora.setTransmitPower(powerConst);
    Lora.writeSettings(TEMPORARY);
    setupSerialInWorkMode();
    loraCurrentPowerConst = Lora.getTransmitPower();
    if (loraCurrentPowerConst != powerConst) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
        return;
    }
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_POWER;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.putULong(preferencesKeyChar, powerConst);
    uint8_t saved = Preferences.getULong(preferencesKeyChar, 0);
    if (saved != powerConst && SerialBluetooth.connected())
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
    Preferences.end();
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_POWER;
        textBuffer = textBuffer + getPowerIndexByConst(loraCurrentPowerConst);
        SerialBluetooth.println(textBuffer);
    }
}

void setupLoraAirSpeed(uint16_t airSpeedIndex) {
    uint8_t airSpeedConst = getLoraAirSpeedConstByIndex(airSpeedIndex);
    if (airSpeedConst == 0) {
        if (SerialBluetooth.connected()) SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_AIR_SPEED);
        return;
    }
    setupSerialInProgramMode();
    Lora.setAirDataRate(airSpeedConst);
    Lora.writeSettings(TEMPORARY);
    setupSerialInWorkMode();
    loraCurrentAirSpeedConst = Lora.getAirDataRate();
    if (loraCurrentAirSpeedConst != airSpeedConst) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
        return;
    }
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_AIR_SPEED;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.putULong(preferencesKeyChar, airSpeedConst);
    uint8_t saved = Preferences.getULong(preferencesKeyChar, 0);
    if (saved != airSpeedConst && SerialBluetooth.connected())
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
    Preferences.end();
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_AIR_SPEED;
        textBuffer = textBuffer + getLoraAirSpeedIndexByConst(loraCurrentAirSpeedConst);
        SerialBluetooth.println(textBuffer);
    }
}

void setupLoraSerialSpeed(uint32_t serialSpeedIndex) {
    uint8_t serialSpeedConst = getLoraSerialSpeedConstByIndex(serialSpeedIndex);
    if (serialSpeedConst == 0) {
        if (SerialBluetooth.connected()) SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_SERIAL_SPEED);
        return;
    }
    setupSerialInProgramMode();
    Lora.setUARTBaudRate(serialSpeedConst);
    Lora.writeSettings(TEMPORARY);
    esp32currentSerialSpeed = serialSpeedIndex;
    setupSerialInWorkMode();
    loraCurrentSerialSpeedConst = Lora.getUARTBaudRate();
    if (loraCurrentSerialSpeedConst != serialSpeedConst) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
        return;
    }
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_SERIAL_SPEED;
        textBuffer = textBuffer + getLoraSerialSpeedIndexByConst(loraCurrentSerialSpeedConst);
        SerialBluetooth.println(textBuffer);
    }
}

void setupLoraNetId(uint8_t netID) {
    if (netID < 0 || netID > 255) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_NET_ID);
        return;
    }
    setupSerialInProgramMode();
    Lora.setNetID(netID);
    Lora.writeSettings(TEMPORARY);
    setupSerialInWorkMode();
    loraCurrentNetIDIndex = Lora.getNetID();
    if (loraCurrentNetIDIndex != netID) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
        return;
    }
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_NET_ID;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.putULong(preferencesKeyChar, netID);
    uint8_t saved = Preferences.getULong(preferencesKeyChar, 0);
    if (saved != netID && SerialBluetooth.connected())
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
    Preferences.end();
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_NET_ID;
        textBuffer = textBuffer + loraCurrentAddressIndex;
        SerialBluetooth.println(textBuffer);
    }
}

void setupLoraAddress(uint16_t address) {
    if (address < 0 || address > 65535) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_ADDRESS);
        return;
    }
    setupSerialInProgramMode();
    Lora.setAddress(address);
    Lora.writeSettings(TEMPORARY);
    setupSerialInWorkMode();
    loraCurrentAddressIndex = Lora.getAddress();
    if (loraCurrentAddressIndex != address) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
        return;
    }
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_ADDRESS;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.putULong(preferencesKeyChar, address);
    uint8_t saved = Preferences.getULong(preferencesKeyChar, 0);
    if (saved != address && SerialBluetooth.connected())
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
    Preferences.end();
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_ADDRESS;
        textBuffer = textBuffer + loraCurrentAddressIndex;
        SerialBluetooth.println(textBuffer);
    }
}

void setupLoraKey(uint16_t keyIndex) {
    if (keyIndex < 0 || keyIndex > 65535) {
        if (SerialBluetooth.connected()) SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_KEY);
        return;
    }
    setupSerialInProgramMode();
    Lora.writeCryptKey(keyIndex, TEMPORARY);
    setupSerialInWorkMode();
    loraCurrentEncryptionKeyIndex = keyIndex;
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_KEY;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.putULong(preferencesKeyChar, keyIndex);
    uint8_t saved = Preferences.getULong(preferencesKeyChar, 0);
    if (saved != keyIndex && SerialBluetooth.connected())
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
    Preferences.end();
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_KEY;
        textBuffer = textBuffer + loraCurrentEncryptionKeyIndex;
        SerialBluetooth.println(textBuffer);
    }
}

void setupLoraChannel(uint16_t channelIndex) {
    if (channelIndex < 0 || channelIndex > 83) {
        if (SerialBluetooth.connected()) SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR_CHANNEL);
        return;
    }
    setupSerialInProgramMode();
    Lora.setChannel(channelIndex);
    Lora.writeSettings(TEMPORARY);
    setupSerialInWorkMode();
    loraCurrentChannelIndex = Lora.getChannel();
    if (loraCurrentChannelIndex != channelIndex) {
        if (SerialBluetooth.connected())SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
        return;
    }
    char preferencesKeyChar[maxKeySize];
    String preferencesKeyString = (__FlashStringHelper *) KEY_LORA_CHANNEL;
    preferencesKeyString.toCharArray(preferencesKeyChar, maxKeySize);
    Preferences.begin(KEY_PREFERENCES, false);
    Preferences.putULong(preferencesKeyChar, channelIndex);
    uint8_t saved = Preferences.getULong(preferencesKeyChar, 0);
    if (saved != channelIndex && SerialBluetooth.connected())
        SerialBluetooth.println((__FlashStringHelper *) MESSAGE_ERROR);
    Preferences.end();
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) COMMAND_GET_RETURN_VALUE_LORA_CHANNEL;
        textBuffer = textBuffer + loraCurrentChannelIndex;
        SerialBluetooth.println(textBuffer);
    }
}

// commands will be json for mobile client
void parseCommand(String buffer) {
    if (buffer.isEmpty()) return;
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_SETTINGS)) {
        printAllLoraSettingsToBluetoothSerial();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_PRINT_COMMANDS)) {
        printAllCommandsToBluetoothSerial();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_FREE_PREFERENCES_ENTRIES)) {
        printFreePreferencesEntries();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_RESET_LORA_SETTINGS)) {
        resetLoraSettings();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_CHANNEL)) {
        printCurrentLoraChannel();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_AIR_SPEED)) {
        printCurrentLoraAirSpeed();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_PACKET_SIZE)) {
        printCurrentLoraPacketSize();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_POWER)) {
        printCurrentLoraPower();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_KEY)) {
        printCurrentLoraKey();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_NET_ID)) {
        printCurrentLoraNetID();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_ADDRESS)) {
        printCurrentLoraAddress();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_LORA_SERIAL_SPEED)) {
        printCurrentLoraSerialSpeed();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_GET_BOARD_SERIAL_SPEED)) {
        printCurrentBoardSerialSpeed();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_NET_ID)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_NET_ID, "");
        setupLoraNetId(buffer.toInt());
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_ADDRESS)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_ADDRESS, "");
        setupLoraAddress(buffer.toInt());
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_KEY)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_KEY, "");
        setupLoraKey(buffer.toInt());
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_POWER)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_POWER, "");
        setupLoraTransmitPower(buffer.toInt());
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_PACKET_SIZE)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_PACKET_SIZE, "");
        setupLoraPacketSize(buffer.toInt());
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_CHANNEL)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_CHANNEL, "");
        setupLoraChannel(buffer.toInt());
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_AIR_SPEED)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_AIR_SPEED, "");
        setupLoraAirSpeed(buffer.toInt());
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SET_LORA_SERIAL_SPEED)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SET_LORA_SERIAL_SPEED, "");
        setupLoraSerialSpeed(buffer.toInt());
        return;
    }
    if (textToSend.isEmpty()) {
        textToSend = buffer;
    } else {
        textToSend = textToSend + "/n" + buffer;
    }
    if (SerialBluetooth.connected()) {
        String textBuffer = (__FlashStringHelper *) MESSAGE_LORA_MESSAGE_QUEUED_FOR_SENDING;
        textBuffer = textBuffer + textToSend;
        SerialBluetooth.println(textBuffer);
    }
    textLastSend = textToSend;
}


void updateSystem() {
    loraIsBusy = Lora.getBusy();
    uint32_t currentTime = millis();
    if (!loraIsBusy && currentTime > timeCodeUpdateAmbient + LORA_UPDATE_AMBIENT_DELAY) {
        loraRssiAmbient = Lora.getRSSI(RSSI_AMBIENT);
        timeCodeUpdateAmbient = currentTime;
        if (SerialBluetooth.connected()) {
            String textBuffer = (__FlashStringHelper *) MESSAGE_LORA_RSSI_AMBIENT;
            textBuffer = textBuffer + loraRssiAmbient;
            SerialBluetooth.println(textBuffer);
        }
    }
    if (Lora.available() > 0) {
        textReceived = Serial1.readString();
        loraRssiLastReceive = Lora.getRSSI(RSSI_LAST_RECEIVE);
        timeCodeReceived = millis();
        if (SerialBluetooth.connected()) {
            String textBuffer = (__FlashStringHelper *) MESSAGE_LORA_MESSAGE_RECEIVED;
            textBuffer = textBuffer + textReceived;
            SerialBluetooth.println(textBuffer);
            textBuffer = (__FlashStringHelper *) MESSAGE_LORA_RSSI_FOR_LAST_MESSAGE_RECEIVED;
            textBuffer = textBuffer + loraRssiLastReceive;
            SerialBluetooth.println(textBuffer);
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
    while (true) {
        updateSystem();
        if (UPDATE_SYSTEM_DELAY > 0)
            delay(UPDATE_SYSTEM_DELAY);
    }
}

void taskUpdateDisplay(void *pvParameters) {
    while (true) {
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
    Serial1.begin(esp32currentSerialSpeed, SERIAL_8N1, PIN_RX, PIN_TX);
    readDataFromEEPROM();
    printAllLoraSettingsToBluetoothSerial();
    setupDisplay();
    Lora.init();
    setupLora();

    setupTasksForESP32Cores();
}

void loop() {
}