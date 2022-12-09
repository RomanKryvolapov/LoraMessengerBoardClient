#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <GyverOLED.h>
#include <EBYTE22.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <BluetoothSerial.h>

#define I2C_SDA 21
#define I2C_SCL 22

#define PIN_TX 17
#define PIN_RX 16
#define PIN_M0 14
#define PIN_M1 12
#define PIN_AX 13
EBYTE22 lora(&Serial1, PIN_M0, PIN_M1, PIN_AX);
GyverOLED<SSH1106_128x64> display;
BluetoothSerial serialBluetooth;

TaskHandle_t Task1;
TaskHandle_t Task2;

const char PROGMEM READY[] = "Ready";
const char PROGMEM COMMAND_SEND[] = "Send ";
const char PROGMEM COMMAND_CONTRAST[] = "Contrast ";
const char PROGMEM COMMAND_CHANNEL[] = "Channel ";
const char PROGMEM COMMAND_RATE[] = "Rate ";
const char PROGMEM COMMAND_PACKET[] = "Packet ";
const char PROGMEM COMMAND_RESET[] = "Reset";
const char PROGMEM COMMAND_POWER[] = "Power ";

const char PROGMEM COMMAND_CONTRAST_ERROR[] = "Must be 0-255";
const char PROGMEM COMMAND_CHANNEL_ERROR[] = "Must be 0-83";
const char PROGMEM COMMAND_RATE_ERROR[] = "Must be 300/1200/2400/4800/9600/19200/38400/62500";
const char PROGMEM COMMAND_PACKET_ERROR[] = "Must be 32/64/128/240";
const char PROGMEM COMMAND_POWER_ERROR[] = "Must be 1/2/3/4";

String textToSend = "";
String lastSend = "";
String textReceived = "";

uint8_t loraRssiLastReceive = 0;
uint8_t loraRssiAmbient = 0;

void updateDisplay() {
    display.clear();
    if (lora.getBusy()) {
        display.setCursorXY(102, 0);
        display.print("BUSY");
    } else {
        display.setCursorXY(102, 0);
        display.print("FREE");
    }
    display.setCursorXY(0, 0);
    display.print("R:");
    display.print(loraRssiLastReceive);
    display.setCursorXY(48, 0);
    display.print("A:");
    display.print(loraRssiAmbient);
    display.setCursorXY(0, 12);
    display.print("> ");
    display.println(textReceived);
    display.setCursorXY(0, 40);
    display.print("< ");
    display.println(lastSend);
    display.update();
}

void configureLora() {
    lora.init();
    lora.setMode(MODE_NORMAL);
    lora.setUARTBaudRate(UBR_9600);
    lora.setParityBit(PB_8N1);
    lora.setAddress(0x0000);
    lora.setNetID(0x00);
    lora.setAirDataRate(ADR_300);
    lora.setPacketLength(PACKET32);
    lora.setRSSIInPacket(RSSI_ENABLE);
    lora.setRSSIAmbient(RSSI_ENABLE);
    lora.setTransmitPower(TP_MAX);
    // 0-83
    lora.setChannel(40);
    lora.setTransmissionMode(TXM_NORMAL);
    lora.setRepeater(REPEATER_DISABLE);
    lora.setLBT(LBT_DISABLE);
    lora.setWOR(WOR_TRANSMITTER);
    lora.setWORCycle(WOR2000);

//    lora.writeSettingsWireless(TEMPORARY);
    lora.writeSettings(TEMPORARY);

//    lora.writeCryptKeyWireless(0x0128, TEMPORARY);
    lora.writeCryptKey(0x0128, TEMPORARY);
}

void taskUpdateDisplay(void *pvParameters) {
    for (;;) {
        updateDisplay();
    }
}

void parseCommand(String buffer) {
    if (buffer.equals((__FlashStringHelper *) COMMAND_RESET)) {
        configureLora();
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_POWER)) {
        buffer.replace((__FlashStringHelper *) COMMAND_POWER, "");
        int power = buffer.toInt();
        switch (power) {
            case 1: {
                lora.setTransmitPower(TP_LOW);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 2: {
                lora.setTransmitPower(TP_MID);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 3: {
                lora.setTransmitPower(TP_HIGH);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 4: {
                lora.setTransmitPower(TP_MAX);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            default: {
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) COMMAND_POWER_ERROR);
                }
                return;
            }
        }
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_PACKET)) {
        buffer.replace((__FlashStringHelper *) COMMAND_PACKET, "");
        int packet = buffer.toInt();
        switch (packet) {
            case 32: {
                lora.setPacketLength(PACKET32);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 64: {
                lora.setPacketLength(PACKET64);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 128: {
                lora.setPacketLength(PACKET128);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 240: {
                lora.setPacketLength(PACKET240);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            default: {
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) COMMAND_PACKET_ERROR);
                }
                return;
            }
        }
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_SEND)) {
        buffer.replace((__FlashStringHelper *) COMMAND_SEND, "");
        if (textToSend.isEmpty()) {
            textToSend = buffer;
            if (serialBluetooth.connected()) {
                serialBluetooth.print("Text: ");
                serialBluetooth.println(textToSend);
            }
        } else {
            textToSend = textToSend + "/n" + buffer;
        }
        lastSend = textToSend;
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_CONTRAST)) {
        buffer.replace((__FlashStringHelper *) COMMAND_CONTRAST, "");
        int contrast = buffer.toInt();
        if (contrast >= 0 && contrast <= 255) {
            display.setContrast(contrast);
            if (serialBluetooth.connected()) {
                serialBluetooth.println((__FlashStringHelper *) READY);
            }
        } else {
            if (serialBluetooth.connected()) {
                serialBluetooth.println((__FlashStringHelper *) COMMAND_CONTRAST_ERROR);
            }
        }
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_CHANNEL)) {
        buffer.replace((__FlashStringHelper *) COMMAND_CHANNEL, "");
        int channel = buffer.toInt();
        if (channel >= 0 && channel <= 83) {
            lora.setChannel(channel);
            lora.writeSettings(TEMPORARY);
            if (serialBluetooth.connected()) {
                serialBluetooth.println((__FlashStringHelper *) READY);
            }
        } else {
            if (serialBluetooth.connected()) {
                serialBluetooth.println((__FlashStringHelper *) COMMAND_CHANNEL_ERROR);
            }
        }
        return;
    }
    if (buffer.startsWith((__FlashStringHelper *) COMMAND_RATE)) {
        buffer.replace((__FlashStringHelper *) COMMAND_RATE, "");
        int rate = buffer.toInt();
        switch (rate) {
            case 300: {
                lora.setAirDataRate(ADR_300);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 1200: {
                lora.setAirDataRate(ADR_1200);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 2400: {
                lora.setAirDataRate(ADR_2400);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 4800: {
                lora.setAirDataRate(ADR_4800);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 9600: {
                lora.setAirDataRate(ADR_9600);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 19200: {
                lora.setAirDataRate(ADR_19200);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 38400: {
                lora.setAirDataRate(ADR_38400);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                }
                return;
            }
            case 62500: {
                lora.setAirDataRate(ADR_62500);
                lora.writeSettings(TEMPORARY);
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) READY);
                    return;
                }
            }
            default: {
                if (serialBluetooth.connected()) {
                    serialBluetooth.println((__FlashStringHelper *) COMMAND_RATE_ERROR);
                }
                return;
            }
        }
    }
}

void taskSystemProcesses(void *pvParameters) {
    for (;;) {
        loraRssiAmbient = lora.getRSSI(RSSI_AMBIENT);
        if (serialBluetooth.connected() && serialBluetooth.available()) {
            parseCommand(serialBluetooth.readString());
        }
        if (lora.available() > 0) {
            textReceived = Serial1.readString();
            loraRssiLastReceive = lora.getRSSI(RSSI_LAST_RECEIVE);
            if (serialBluetooth.connected()) {
                serialBluetooth.println();
                serialBluetooth.print("Received: ");
                serialBluetooth.println(textReceived);
                serialBluetooth.print("RSSI received: ");
                serialBluetooth.println(loraRssiLastReceive);
                serialBluetooth.print("RSSI ambient: ");
                serialBluetooth.println(loraRssiAmbient);
                serialBluetooth.println();
            }
        }
        if (!textToSend.isEmpty() && Serial1.available()) {
            Serial1.print(textToSend);
            textToSend = "";
        }
        delay(100);
    }
}

void configureDisplay() {
    display.init();
    display.clear();
    display.autoPrintln(true);
    display.setContrast(255);
    display.setScale(4);
    display.setCursorXY(5, 20);
    display.invertDisplay(true);
    display.print("Start");
    display.update();
    display.setScale(1);
    delay(1000);
    display.invertDisplay(false);
}

void setup() {
    serialBluetooth.begin("ESP32MESSENGER");
    Serial1.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);
    Wire.begin(I2C_SDA, I2C_SCL);

    configureLora();
    configureDisplay();

    xTaskCreatePinnedToCore(
            taskUpdateDisplay,   /* Функция задачи */
            "Task1",     /* Название задачи */
            10000,       /* Размер стека задачи */
            NULL,        /* Параметр задачи */
            1,           /* Приоритет задачи */
            &Task1,      /* Идентификатор задачи,
                                    чтобы ее можно было отслеживать */
            0);          /* Ядро для выполнения задачи (0) */
    xTaskCreatePinnedToCore(
            taskSystemProcesses,   /* Функция задачи */
            "Task2",     /* Название задачи */
            10000,       /* Размер стека задачи */
            NULL,        /* Параметр задачи */
            1,           /* Приоритет задачи */
            &Task2,      /* Идентификатор задачи,
                                    чтобы ее можно было отслеживать */
            1);          /* Ядро для выполнения задачи (1) */
}

void loop() {

}