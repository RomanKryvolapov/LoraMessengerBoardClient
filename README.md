<h1>Lora Messenger</h1>
<p>This project allows you to communicate through Ebyte's Lora E22 modules using a smartphone with Bluetooth and ESP32.</p>
<p>I managed to establish a connection at a distance of 8-12 km with a device located on the balcony of the 4th floor of the house.</p>
<p>This project is for people who currently do not have access to the Internet and mobile network, but who want to stay connected with other people who have this device.</p>
<p>In the future, a client for a smartphone and the ability to use a keyboard will be added to the project.</p>
<p>At the moment, it is possible to communicate with the device using commands via a COM port and a Bluetooth client for a mobile phone, such as the Serial Bluetooth Terminal application.</p>
<p>Tested with E22 UART 400MHz 900MHz 230MHz modules, support for E32, E220 and other uart modules will be added in the future.</p>
<p>For the mobile client, the JSON format will be used, as well as the ability to connect via Bluetooth BLE.</p>
<p>Made with CLion / Platformio.</p>

![Device 1!](https://github.com/RomanKryvolapov/LoraMessenger/blob/main/IMG_20221214_230954.jpg "Device 1")

![Device 2!](https://github.com/RomanKryvolapov/LoraMessenger/blob/main/IMG_20221214_231034.jpg?raw=true "Device 2")

<p>Commands:</p>
<p>Send ... (some text)</p>
<p>Channel ... (0-83)</p>
<p>Rate ... (300/1200/2400/4800/9600/19200/38400/62500)</p>
<p>Packet ... (32/64/128/240)</p>
<p>Reset</p>
<p>Power ... (1/2/3/4)</p>
<p>Key ... (0-65535)</p>
<p>NetID ... (0-255)</p>
<p>Address ... (0-655350-65535, address 65535 is used for broadcast messages. With this address, the module will receive all messages, and messages sent from this module will be accepted by all other modules, regardless of the address settings on them.)</p>