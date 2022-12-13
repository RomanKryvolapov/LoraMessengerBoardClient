#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <EBYTE22.h>
#include <HardwareSerial.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>

// Display (size 10 x 20 symbols)
#define TFT_CS 5
#define TFT_RST 4
#define TFT_DC 2

// I2C
//#define I2C_SDA 21
//#define I2C_SCL 22

// Lora
#define PIN_TX 17
#define PIN_RX 16
#define PIN_M0 14
#define PIN_M1 12
#define PIN_AX 13

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

const char PROGMEM ERROR_CHANNEL[] = "Must be 0-83";
const char PROGMEM ERROR_AIR_RATE[] = "Must be 300/1200/2400/4800/9600/19200/38400/62500";
const char PROGMEM ERROR_PACKET_SIZE[] = "Must be 32/64/128/240";
const char PROGMEM ERROR_POWER[] = "Must be 1/2/3/4";
const char PROGMEM ERROR_KEY[] = "Must be 0-65535";
const char PROGMEM ERROR_NET_ID[] = "Must be 0-255";
const char PROGMEM ERROR_ADDRESS[] = "Must be 0-65535";

const char ADDRESS_CHANNEL[] = "CHANNEL";
const char ADDRESS_RATE[] = "RATE";
const char ADDRESS_PACKET[] = "PACKET";
const char ADDRESS_POWER[] = "POWER";
const char ADDRESS_KEY[] = "KEY";
const char ADDRESS_NET_ID[] = "NET_ID";
const char ADDRESS_ADDRESS[] = "ADDRESS";

const uint8_t STEP_LINE = 16;
const uint8_t STEP_SYMBOL = 12;

const uint32_t UPDATE_DISPLAY_DELAY = 100;
const uint32_t UPDATE_SYSTEM_DELAY = 0;
const uint32_t taskSystemProcessesStack = 50000;
const uint32_t taskUpdateDisplayStack = 50000;
const uint32_t updateAmbientDelay = 5000;

bool loraIsBusy = false;

String textToSend = "";
String lastSend = "";
String textReceived = "";
uint32_t timeCodeReceived = 0;

uint8_t loraRssiLastReceive = 0;
uint8_t loraRssiAmbient = 0;

uint8_t currentChannel = 40;
uint8_t currentAirDataRate = ADR_2400;
uint8_t currentPacketSize = PACKET240;
uint8_t currentUartSpeed = UBR_9600;
uint16_t currentUartSerialSpeed = 9600;
uint8_t currentPower = TP_MAX;
uint16_t currentKey = 0x0000;
uint16_t currentAddress = 0x0000;
uint8_t currentNetID = 0x00;
uint32_t updateAmbientTimeCode = 0;

uint16_t getLine(uint8_t numberOfLine) {
    return numberOfLine * STEP_LINE;
}

uint8_t getSymbol(uint8_t numberOfSymbol) {
    return numberOfSymbol * STEP_SYMBOL;
}

void readDataFromEEPROM() {
    Preferences.begin("Preferences", false);
    uint16_t channel = Preferences.getUShort(ADDRESS_CHANNEL, 65535);
    if (channel != 65535) {
        currentChannel = channel;
    }
    uint16_t airDataRate = Preferences.getUShort(ADDRESS_RATE, 65535);
    if (airDataRate != 65535) {
        currentAirDataRate = airDataRate;
    }
    uint16_t packetSize = Preferences.getUShort(ADDRESS_PACKET, 65535);
    if (packetSize != 65535) {
        currentPacketSize = packetSize;
    }
    uint16_t power = Preferences.getUShort(ADDRESS_POWER, 65535);
    if (power != 65535) {
        currentPower = power;
    }
    uint32_t address = Preferences.getUInt(ADDRESS_POWER, 4294967295);
    if (address != 4294967295) {
        currentAddress = address;
    }
    uint32_t key = Preferences.getUInt(ADDRESS_KEY, 4294967295);
    if (key != 4294967295) {
        currentKey = key;
    }
    uint16_t netID = Preferences.getUShort(ADDRESS_NET_ID, 65535);
    if (netID != 65535) {
        currentNetID = netID;
    }
}

void printText(String text, uint8_t line, uint8_t position, uint8_t clearSymbolsX, uint8_t clearSymbolsY) {
    Display.setCursor(getSymbol(position), getLine(line));
    Display.fillRect(
            getSymbol(position),
            getLine(line),
            getSymbol(clearSymbolsX),
            STEP_LINE * clearSymbolsY,
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
    printText(String("Chan: ") + currentChannel, 1, 10, 10, 1);
    switch (currentAirDataRate) {
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
    switch (currentPacketSize) {
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
    printText(String("Key: ") + ltoa(currentKey, keyHex, 16), 3, 0, 10, 1);
    switch (currentPower) {
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
    printText(String("Addr:") + ltoa(currentAddress, addressHex, 16), 4, 0, 10, 1);
    char netIDHex[10] = "";
    printText(String("NetID:") + ltoa(currentNetID, netIDHex, 16), 4, 10, 10, 1);
    printText(String(timeCodeReceived), 5, 0, 20, 1);
    printText(textReceived, 6, 0, 20, 14);
}

void setupLora() {
    Lora.setUARTBaudRate(currentUartSpeed);
    Lora.setMode(MODE_NORMAL);
    Lora.setParityBit(PB_8N1);
    Lora.setTransmissionMode(TXM_NORMAL);
    Lora.setRepeater(REPEATER_DISABLE);
    Lora.setLBT(LBT_DISABLE);
    Lora.setWOR(WOR_TRANSMITTER);
    Lora.setWORCycle(WOR2000);
    Lora.setRSSIInPacket(RSSI_ENABLE);
    Lora.setRSSIAmbient(RSSI_ENABLE);
    Lora.setTransmitPower(currentPower);
    Lora.setChannel(currentChannel);
    Lora.setAddress(currentAddress);
    Lora.setNetID(currentNetID);
    Lora.setAirDataRate(currentAirDataRate);
    Lora.setPacketLength(currentPacketSize);
    Lora.writeSettings(TEMPORARY);
    Lora.writeCryptKey(currentKey, TEMPORARY);
//    Lora.writeSettingsWireless(TEMPORARY);
//    Lora.writeCryptKeyWireless(0x0128, TEMPORARY);
}

void parseCommand(String buffer) {
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SEND)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SEND, "");
        if (textToSend.isEmpty()) {
            textToSend = buffer;
            if (SerialBluetooth.connected()) {
                SerialBluetooth.print("Text to send: ");
                SerialBluetooth.println(textToSend);
            }
        } else {
            textToSend = textToSend + "/n" + buffer;
        }
        lastSend = textToSend;
        return;
    }
    if (buffer.equals((__FlashStringHelper *) COMMAND_RESET)) {
        Preferences.clear();
        currentChannel = 40;
        currentAirDataRate = ADR_300;
        currentPacketSize = PACKET32;
        currentPower = TP_MAX;
        currentKey = 128;
        currentAddress = 0;
        currentNetID = 0;
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
            currentNetID = netID;
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
            currentAddress = address;
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
            currentKey = key;
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
                currentPower = TP_LOW;
                Lora.setTransmitPower(TP_LOW);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_POWER, TP_LOW);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 2: {
                currentPower = TP_MID;
                Lora.setTransmitPower(TP_MID);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_POWER, TP_MID);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 3: {
                currentPower = TP_HIGH;
                Lora.setTransmitPower(TP_HIGH);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_POWER, TP_HIGH);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 4: {
                currentPower = TP_MAX;
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
                currentPacketSize = PACKET32;
                Lora.setPacketLength(PACKET32);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_PACKET, PACKET32);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 64: {
                currentPacketSize = PACKET64;
                Lora.setPacketLength(PACKET64);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_PACKET, PACKET64);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 128: {
                currentPacketSize = PACKET128;
                Lora.setPacketLength(PACKET128);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_PACKET, PACKET128);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 240: {
                currentPacketSize = PACKET240;
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
            currentChannel = channel;
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
                currentAirDataRate = ADR_300;
                Lora.setAirDataRate(ADR_300);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_300);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 1200: {
                currentAirDataRate = ADR_1200;
                Lora.setAirDataRate(ADR_1200);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_1200);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 2400: {
                currentAirDataRate = ADR_2400;
                Lora.setAirDataRate(ADR_2400);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_2400);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 4800: {
                currentAirDataRate = ADR_4800;
                Lora.setAirDataRate(ADR_4800);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_4800);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 9600: {
                currentAirDataRate = ADR_9600;
                Lora.setAirDataRate(ADR_9600);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_9600);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 19200: {
                currentAirDataRate = ADR_19200;
                Lora.setAirDataRate(ADR_19200);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_19200);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 38400: {
                currentAirDataRate = ADR_38400;
                Lora.setAirDataRate(ADR_38400);
                Lora.writeSettings(TEMPORARY);
                Preferences.putUShort(ADDRESS_RATE, ADR_38400);
                if (SerialBluetooth.connected()) {
                    SerialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 62500: {
                currentAirDataRate = ADR_62500;
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

void updateSystem() {
    loraIsBusy = Lora.getBusy();
    uint32_t currentTime = millis();
    if (!loraIsBusy && currentTime > updateAmbientTimeCode + updateAmbientDelay) {
        loraRssiAmbient = Lora.getRSSI(RSSI_AMBIENT);
        updateAmbientTimeCode = currentTime;
    }
    if (Lora.available() > 0) {
        textReceived = Serial1.readString();
        loraRssiLastReceive = Lora.getRSSI(RSSI_LAST_RECEIVE);
        timeCodeReceived = millis();
        if (SerialBluetooth.connected()) {
            SerialBluetooth.println();
            SerialBluetooth.print("Received: ");
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
            SerialBluetooth.print("Send: ");
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

void setup() {
    SerialBluetooth.begin("ESP32MESSENGER");
    Serial1.begin(currentUartSerialSpeed, SERIAL_8N1, PIN_RX, PIN_TX);

    readDataFromEEPROM();
    setupDisplay();
    Lora.init();
    setupLora();

    xTaskCreatePinnedToCore(
            taskUpdateDisplay,
            "taskUpdateDisplay",
            taskUpdateDisplayStack,
            NULL,
            1,
            &TaskUpdateDisplay,
            0);
    xTaskCreatePinnedToCore(
            taskSystemProcesses,
            "taskSystemProcesses",
            taskSystemProcessesStack,
            NULL,
            1,
            &TaskSystemProcesses,
            1);

}

void loop() {
}