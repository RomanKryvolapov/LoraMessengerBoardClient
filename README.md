<h3>Lora Messenger</h2>
<p>This project is for people who currently do not have access to the Internet and mobile network, but who want to stay connected with other people who have this device.</p>
<p>This project allows you to communicate through Lora modules (cost about 3-10 euros) using a smartphone or computer supported Bluetooth Serial Port, and ESP32 (board with 2x core processor, WI-FI and Bluetooth, compatible with the Platformio development environment, cost about 3 euros) or ESP8266 or any other board  with Bluetooth, supported by the Platformio development environment https://platformio.org/</p>
<p>I tested the range in urban conditions, and I was able to establish communication from different parts of the city with a module located at a distance of 8-12 kilometers at the level of the 4th floor outside the line of sight, this indicates a good potential for communication range.</p>
<p>It is possible to communicate with device using commands via a Bluetooth Serial Port, and you can use Bluetooth Serial Port application such as the Serial Bluetooth Terminal application for Android or YAT for Windows. Below is a list of supported commands, it will be updated. Commands need to be sent as normal text messages through the terminal, you will receive responses to them in the form of messages.</p>
<p>Simultaneously with this project, I am also making a mobile client for Android, in which the interaction will be similar to a messenger as Telegram or WhatsApp.</p>
<p>But, my idea is that if you do not have access to a smartphone, but have access to any device that supports Bluetooth Serial port, you can still make settings and receive and send messages in command line communication mode. This is the reason why I do not want to organize communication between board and the smartphone using JSON and Bluetooth Low Energy.</p>
<p>I used a 320x240 screen to display the parameters of the module and incoming messages, but it is possible to use any screen with an interface I2C/SPI. In the future, I want to connect a keyboard to board so that I can make a menu for settings and so that I can send messages without using a smartphone.</p>
<p>Currently working with modules produced by Ebyte 400MHz / 900MHz / 230MHz / 170MHz, because they are among the best in range and at the same time they are cheap. In the future, there will be compatibility with modules from other manufacturers.</p>
<p>Made with CLion / Platformio.</p>
<h3>Following commands are currently supported:</h3>
<h4>SetChannel:...</h4>
<p>E22-400T30D 400mhz: 0-83, Frequency= 410.125 + CH*1M</p>
<p>E22-900T30D 900mhz: 0-80, Frequency= 850.125 + CH*1M</p>
<p>E22-230T30D 230mhz: 0-83, Frequency = 220.125 + CH*0.25M</p>
<p>The remaining modules support about 80 channels</p>
<h4>SetAirSpeed:...</h4>
<p>300/1200/2400/4800/9600/19200/38400/62500, 62500 = 62.5kbs, some modules do not support 62500</p>
<h4>SetSerialSpeed:...</h4>
<p>1200/2400/4800/9600/19200/38400/57600/115200б the module is configured at the port frequency 9600</p>
<h4>SetPacketSize:...</h4>
<p>32/64/128/240, packet size in bytes</p>
<h4>SetPower:...</h4>
<p>1/2/3/4, E22-...T30D 30dbi: 21dbm 24dbm 27dbm 30dbm, E22-...T22D 22dbi: 10dbm 13dbm 17dbm 22dbm</p>
<h4>SetKey:...</h4>
<p>0-65535, encryption key for Lora module, must be the same for 2 modules, mobile client will use AES encryption</p>
<h4>SetNetID:...</h4>
<p>0-255, must be the same for 2 modules</p>
<h4>SetAddress:...</h4>
<p>0-65535, address 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.</p>
<h4>SetBluetoothName:...</h4>
<h4>SetBluetoothPin:...</h4>
<h3>You can get the current module settings with the commands:</h3>
<h4>GetChannel</h4>
<h4>GetAirSpeed</h4>
<h4>GetSerialSpeed</h4>
<h4>GetBoardSerialSpeed</h4>
<h4>GetPacketSize</h4>
<h4>GetPower</h4>
<h4>GetKey</h4>
<h4>GetNetID</h4>
<h4>GetAddress</h4>
<h4>GetMode</h4>
<h4>GetParityBit</h4>
<h4>GetTransmissionMode</h4>
<h4>GetRepeaterMode</h4>
<h4>GetMonitoringBeforeDataTransmissionMode</h4>
<h4>GetWORMode</h4>
<h4>GetWORCycle</h4>
<h4>GetRSSItoTheEndOfReceivedDataMode</h4>
<h4>GetListeningToAirAmbientMode</h4>
<h4>GetSavingSettingsMemory</h4>
<h4>GetFreeRamMemory</h4>
<h4>GetFreePreferencesEntries</h4>
<h4>GetSettings (print all settings)</h4>
<h4>ResetSettings (reset to default settings, saved settings are also erased)</h4>
<h4>PrintCommands (Print all available commands)</h4>

![Device 1!](https://github.com/RomanKryvolapov/LoraMessenger/blob/main/IMG_20221214_230954.jpg "Device 1")

![Device 2!](https://github.com/RomanKryvolapov/LoraMessenger/blob/main/IMG_20221214_231034.jpg?raw=true "Device 2")


