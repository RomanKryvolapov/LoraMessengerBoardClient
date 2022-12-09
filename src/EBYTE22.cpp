/*
  The MIT License (MIT)
  Copyright (c) 2021 TryTech
  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  On a personal note, if you develop an application or product using this library 
  and make millions of dollars, I'm happy for you!
*/

/* 
  Библиотека служит для работы с радиомодулями модели E22 компании EBYTE.
  Эти радиомодули используют LoRa модуляцию для организации дальней связи.
  Библиотека написана автором YouTube канала TryTech  http://www.youtube.com/c/TryTech

  Данная библиотека была основана на библиотеке "EBYTE" от KrisKasprzak 
  Ссылка на источник библиотеки https://github.com/KrisKasprzak/EBYTE
  В отличии от предыдущих моделей, модули E22 имеют больший функционал,
  а взаимодействии с ними сильно отличается, поэтому многие методы были
  полностью переписаны и добавлены новые. Но все же я благодарен автору
  библиотеки указанной выше, благодаря ему я сэкономил немного времени.
   
  Версия		Дата			Автор			Описание
  1.0			10/3/2020		TryTech			Создание библиотеки
*/

#include <EBYTE22.h>
#include <Stream.h>

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/*
	Создаем объект класса EBYTE22
	В качестве первого аргумента может быть адрес объекта Serial(аппаратный UART) или SoftwareSerial (программный UART)
*/

EBYTE22::EBYTE22(Stream *s, uint8_t PIN_M0, uint8_t PIN_M1, uint8_t PIN_AUX)

{

	_s = s;
	_M0 = PIN_M0;
	_M1 = PIN_M1;
	_AUX = PIN_AUX;
	_s->setTimeout(1000);
		
}
/*
	Метод инициализации. Служит для конфигурации выводов микроконтроллера и считывания настроек из модуля в библиотеку.
	Для начала работы с модулем нужно обязательно вызвать инициализацию.
	Если параметры не будут считаны из модуля, вернет false (например, если модуль неправильно подключен)
	При инициализации и некоторых других операциях скорость UART должна быть 9600.
*/
bool EBYTE22::init() {
	pinMode(_AUX, INPUT);
	pinMode(_M0, OUTPUT);
	pinMode(_M1, OUTPUT);
	setMode(MODE_NORMAL);
	delay(100);

	if (!readSettings()) {
		return false;
	}

	return true;
}


/*
	Показывает, есть ли непрочитанные данные
*/

bool EBYTE22::available() {

	return _s->available();
	
}

// Ожидать окончание передачи данных по UART от микроконтроллера к модулю.

void EBYTE22::flush() {

	_s->flush();
	
}

/*
	Отправить один байт.
*/

void EBYTE22::sendByte( uint8_t TheByte) {

	_s->write(TheByte);
	
}


/*
	Прочитать один байт.
*/

uint8_t EBYTE22::getByte() {
	 _s->readBytes((uint8_t*)&_buf, (uint8_t) 1);
	 return _buf;
	//return _s->read();

}


/*
	Отправить структуру данных. Первый аргумент - адрес структуры (например &myStruct,
	если не понятно, зачем перед myStruct стоит &, изучите работу с указателями C++)
	Второй аргумент - размер структуры в байтах, можете получить с помощью оператора sizeof(),
	например, sizeof(myStruct).
	Помните, что у разных микроконтроллеров данные одного типа могут иметь разный размер.
	В таком случае, при чтении отправленной структуры, данные могут быть повреждены.

	Структуры - отличный способ обмена информацией между микроконтроллерами.
	Например, у вас есть значения с метеодатчика (температура, влажность и давление).
	Вы можете поместить эти данные в структуру и отправить ее. Принимающая сторона
	считывает структуру методом getStruct и сразу получает все 3 значения в виде переменных,
	не нужно читать буфер байт и "раскладывать" его по переменным.

*/

bool EBYTE22::sendStruct(const void *TheStructure, uint16_t size_) {
		
		completeTask(1000);
		_buf = _s->write((uint8_t *) TheStructure, size_);
		
		
		return (_buf == size_);

}


/*
	Чтение структуры. Структуры отправляющей и принимающей стороны должны содержать одинаковые элементы,
	иначе это теряет смысл.
*/


bool EBYTE22::getStruct(const void *TheStructure, uint16_t size_) {
	
	_buf = _s->readBytes((uint8_t *) TheStructure, size_);

	

	return (_buf == size_);
}

/*
	Отправить адрес и канал получателя (работает только в режиме адресной передачи).
	После отправки адреса, данные должны быть отправлены сразу же, без задержки, иначе
	передатчик оправит по адресу пустой пакет и будет ожидать ввода нового адреса.
	Чтобы передать данные на новый адрес, нужно удостовериться, что модуль закончил
	работу со старым. Адрес 0xFFFF (65535) служит для широковещательной передачи (для всех
	модулей в этой сети на указанном канале).
*/

bool EBYTE22::sendTarget(uint16_t targetAddres, uint8_t targetChannel,  unsigned long timeout) {
if(timeout>0) completeTask(timeout);
if(_TXMode!=TXM_FIXED) return false;
if(!getBusy()){
	uint8_t aH = ((targetAddres & 0xFFFF) >> 8);
	uint8_t aL = (targetAddres & 0xFF);

	_s->write(aH);
	_s->write(aL);
	_s->write(targetChannel);
	return true;
	}else{ 
	return false;
	}

}


/*
	В некоторых случаях(см. техническую информацию) модуль выдает низкий уровень сигнала на выводе AUX.
	Это значит, что в данный момент модуль занят и не может принимать данные по UART или немедленно
	сменить режим работы. Следующий метод служит для ожидания завершения "занятости" модуля.
	Чтобы предотвратить зависание, предусмотрен таймаут, на случай бесконечного низкого сигнала на выводе AUX.
	Иногда модуль "говорит" что он "свободен", но через короткое время (обычно около 0.5 мс) снова становится "занят".
	Я долго не мог понять в чем дело, пока не подключил осциллограф.
	Теперь метод написан так, чтобы предотвратить эту ситуацию.
*/

void EBYTE22::completeTask(unsigned long timeout) {

	unsigned long t = millis();

	if (((unsigned long) (t + timeout)) == 0){
		t = 0;
	}

	if (_AUX != -1) {
		
	
		while (true) {
			if(digitalRead(_AUX) == HIGH){
			delay(2);
			if(digitalRead(_AUX) == HIGH){
			delay(3);
			break;
			}
			}
			
			if ((millis() - t) > timeout){
				break;
			}
			yield();
		}
	}
	else {
		// На случай, если вы не используете вывод AUX, вызывается эта задержка в 1 секунду.
		// Вы можете увеличить это значение, это повысит стабильность, но внесет дополнительную
		// задержку при выполнении некоторых операций.
		delay(1000);

	}

	delay(1);
}

/*
	Установка режима работы, смотрите константы в .h файле.
*/

void EBYTE22::setMode(uint8_t mode) {
	
	
	if (mode == MODE_NORMAL) {
		digitalWrite(_M0, LOW);
		digitalWrite(_M1, LOW);
	}
	else if (mode == MODE_WOR) {
		digitalWrite(_M0, HIGH);
		digitalWrite(_M1, LOW);
	}
	else if (mode == MODE_SLEEP) {
		digitalWrite(_M0, HIGH);
		digitalWrite(_M1, HIGH);
	}
	else if (mode == MODE_CONFIG) {
		digitalWrite(_M0, LOW);
		digitalWrite(_M1, HIGH);
	}

	// 	После пробуждения из режима сна или WOR, нужно немного подождать.
	
	if(_mode == MODE_WOR){
	delay(RECOVERY_AFTER_WOR);
	} else {
	if(_mode == MODE_SLEEP){
	delay(RECOVERY_AFTER_SLEEP);
	}
	}
	_mode=mode;

	// Задержка после переключения режима, вы можете изменить значение константы в .h файле.
	
	delay(MODE_RECOVER);
	completeTask(500);
	
}

// Возвращает текущий режим (см. константы в .h файле)

uint8_t EBYTE22::getMode() {
	
	return _mode;
	
}

// Возвращает true, если модуль сейчас чем-то занят. Желательно делать проверку перед отправкой данных.

bool EBYTE22::getBusy() {
	if(digitalRead(_AUX) == HIGH){
		return false;
	} else return true;

}

	// Методы для установки и получения значений настроек. ВНИМАНИЕ! Эти методы всего лишь меняют значения настроек в библиотеке, чтобы
	// их применить, нужно отправить их в модуль.

// Верхний (первый) байт адреса 0-255

void EBYTE22::setAddressH(uint8_t AddressH) {
	_AddressHigh = AddressH;
}

uint8_t EBYTE22::getAddressH() {
	return _AddressHigh;
}

// Нижний (второй) байт адреса 0-255

void EBYTE22::setAddressL(uint8_t AddressL) {
	_AddressLow = AddressL;
}


uint8_t EBYTE22::getAddressL() {
	return _AddressLow;
}

/*
	Адрес одним числом 0-65535, включает сразу верхний и нижний байт адреса, может кому - то так удобнее.
*/

void EBYTE22::setAddress(uint16_t Address) {
	_AddressHigh = ((Address & 0xFFFF) >> 8);
	_AddressLow = (Address & 0xFF);
}

uint16_t EBYTE22::getAddress() {
	return (_AddressHigh << 8) | (_AddressLow );
}

/*
	Канал радиосвязи 0-83 для модуля на 433MHz (У модулей на другие диапазоны количество каналов отличается), от него зависит частота передачи/приема.
*/

void EBYTE22::setChannel(uint8_t channel) {
	_Channel = channel;
}
uint8_t EBYTE22::getChannel() {
	return _Channel;
}


/*
	Скорость передачи данных по радиоканалу (см. константы в .h файле), для модулей, "общающихся"
	между собой, должна быть одинаковой.
*/

void EBYTE22::setAirDataRate(uint8_t adr) {

	_AirDataRate = adr;
	buildSpeedByte();
	
}

uint8_t EBYTE22::getAirDataRate() {
	return _AirDataRate;
}


/*
	Бит четности (см. константы в .h файле)
*/


void EBYTE22::setParityBit(uint8_t pb) {
	_ParityBit = pb;
	buildSpeedByte();
}
uint8_t EBYTE22::getParityBit( ) {
	return _ParityBit;
}

/*
	Режим передачи, адресный (с обязательным указанием цели) или нормальный (на тот же адрес, на который настроен модуль) (см. константы в .h файле)
*/

void EBYTE22::setTransmissionMode(uint8_t txm) {
	_TXMode = txm;
	buildOptionByte();
}
uint8_t EBYTE22::getTransmissionMode( ) {
	return _TXMode;
}

// Идентификатор сети, должен быть одинаковым для модулей, "общающихся" между собой.

void EBYTE22::setNetID(uint8_t netID) {
	_NetID = netID;
}
uint8_t EBYTE22::getNetID( ) {
	return _NetID;
}

// Работа в режиме WOR, передача или прием (см. константы в .h файле). 

void EBYTE22::setWOR(uint8_t wor) {
	_WOR = wor;
	buildOptionByte();
}
uint8_t EBYTE22::getWOR( ) {
	return _WOR;
}

// Цикл режима WOR (см. константы в .h файле). Должен быть одинаковым для модулей, "общающихся" между собой.

void EBYTE22::setWORCycle(uint8_t worCycle) {
	_WORCycle = worCycle;
	buildOptionByte();
}
uint8_t EBYTE22::getWORCycle() {
	return _WORCycle;
}

// Максимальная длина одного пакета (см. константы в .h файле). Должна быть одинаковой для модулей, "общающихся" между собой.

void EBYTE22::setPacketLength(uint8_t packet) {
	_PacketLength = packet;
	buildPacketByte();
}
uint8_t EBYTE22::getPacketLength( ) {
	return _PacketLength;
}

// Определяет возможность запросить RSSI в нормальном режиме или в режиме WOR (см. константы в .h файле).

void EBYTE22::setRSSIAmbient(uint8_t rssi) {
	_RSSIAmbient = rssi;
	buildPacketByte();
}
uint8_t EBYTE22::getRSSIAmbient( ) {
	return _RSSIAmbient;
}

// Если разрешено, в конце принятых данных добавляется 1 байт, содержащий значение RSSI (см. константы в .h файле).

void EBYTE22::setRSSIInPacket(uint8_t rssi) {
	_RSSIInPacket = rssi;
	buildOptionByte();
}
uint8_t EBYTE22::getRSSIInPacket( ) {
	return _RSSIInPacket;
}

// Функция репитера (см. константы в .h файле).

void EBYTE22::setRepeater(uint8_t repeater) {
	_Repeater = repeater;
	buildOptionByte();
}
uint8_t EBYTE22::getRepeater( ) {
	return _Repeater;
}

// Мониторинг канала перед передачей данных (см. константы в .h файле).

void EBYTE22::setLBT(uint8_t lbt) {
	_LBT = lbt;
	buildOptionByte();
}
uint8_t EBYTE22::getLBT( ) {
	return _LBT;
}

// Мощность передатчика (см. константы в .h файле). Изучите законодательство вашей страны,
// касаемо разрешенной мощности в определенных частотных диапазонах.

void EBYTE22::setTransmitPower(uint8_t tp) {
	_Power = tp;
	buildPacketByte();
}

uint8_t EBYTE22::getTransmitPower() {
	return _Power;
}

//	Скорость передачи данных по UART, бод/сек (см. константы в .h файле), в режиме конфигурации всегда используется 9600.

void EBYTE22::setUARTBaudRate(uint8_t ubr) {
	_UARTDataRate = ubr;
	buildSpeedByte();
}

uint8_t EBYTE22::getUARTBaudRate() {
	return _UARTDataRate;
}

/*
	Получить значение RSSI окружающей среды или последнего приема данных (см. константы в .h файле).
	Работает в нормальном и WOR режиме. Возвращает значение в "попугаях".
	Производитель говорит, что получить значение в dBm можно так: -RSSI/2
	Но я заметил, что в таком случае чем лучше сигнал, тем больше это значение
	уходит в минус, а должно быть наоборот.
*/

uint8_t EBYTE22::getRSSI(uint8_t rssi) {
	if((_mode==MODE_NORMAL||_mode==MODE_WOR)&&_RSSIAmbient==RSSI_ENABLE){
	completeTask(1000);
	clearBuffer();
		
	_Params[0] = 0xC0;
	_Params[1] = 0xC1;
	_Params[2] = 0xC2;
	_Params[3] = 0xC3;
	_Params[4] = rssi;
	_Params[5] = 0x01;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	
	_s->write((uint8_t*)&_Params, (uint8_t) 6);
	_s->flush();
	delay(10);
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_buf, (uint8_t) 1);
	//Serial.println("HEAD");
	//Serial.println(_Head[0]);
	//Serial.println(_Head[1]);
	//Serial.println(_Head[2]);
	//Serial.println("Buf");
	//Serial.println(_buf);
	
	// В моей версии модулей допущена ошибка, при проверке RSSI последнего принятого пакета
	// модуль не отправляет первый байт ответа "0xC1". Следующее условие учитывает эту ошибку.
	
	if (0x01 == _Head[0] && 0x01 == _Head[1] && rssi == RSSI_LAST_RECEIVE){
	
	completeTask(200);
		return _Head[2];
	}
	
	if (0xC1 != _Head[0] || rssi != _Head[1] || _Head[2] != 0x01){
	
	completeTask(200);
		return 0;
	}
	completeTask(200);
	return _buf;
	
	}
return 0;
}



/*
	При считывании настроек с модуля, некоторые отдельные байты содержат значения сразу нескольких параметров, поэтому "разбиваются" на несколько переменных.
	Эти методы "собирают" эти настройки обратно в "свои" байты, потому что только в таком виде их можно записать обратно в модуль.
*/

void EBYTE22::buildSpeedByte() {
	_Speed = 0;
	_Speed = ((_UARTDataRate & 0xFF) << 5) | ((_ParityBit & 0xFF) << 3) | (_AirDataRate & 0xFF);
}


void EBYTE22::buildOptionByte() {
	_Options = 0;
	_Options = ((_RSSIInPacket & 0xFF) << 7) | ((_TXMode & 0xFF) << 6) | ((_Repeater & 0xFF) << 5) | ((_LBT & 0xFF) << 4) | ((_WOR & 0xFF) << 3) | (_WORCycle&0b111);
}

void EBYTE22::buildPacketByte() {
	_Packet = 0;
	_Packet = ((_PacketLength & 0xFF) << 6) | ((_RSSIAmbient & 0xFF) << 5) | (_Power & 0xFF);
}

/*
	Запись настроек в модуль. Можно применить временно или записать значения в ПЗУ модуля (см. константы в .h файле).
*/

bool EBYTE22::writeSettings(uint8_t memory) {
	
	completeTask(MAX_WAIT_TIME);
	
	if(!getBusy()){
	uint8_t currentMode = getMode();
	setMode(MODE_CONFIG);
	completeTask(1000);

	clearBuffer();	
	
	delay(5);

	_s->write(memory);
	
	_s->write((uint8_t)0x00);
	
	_s->write((uint8_t)0x07);

	_s->write(_AddressHigh);

	_s->write(_AddressLow);

	_s->write(_NetID);

	_s->write(_Speed);

	_s->write(_Packet);
	
	_s->write(_Channel);

	_s->write(_Options);

	delay(10);
	completeTask(1000);
	
	_Params[0] = 0;
	_Params[1] = 0;
	_Params[2] = 0;
	_Params[3] = 0;
	_Params[4] = 0;
	_Params[5] = 0;
	_Params[6] = 0;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	_s->setTimeout(6000);
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_Params, (uint8_t) sizeof(_Params));
	_s->setTimeout(1000);
	setMode(currentMode);
	//completeTask(1000);
	if (0xC1 != _Head[0] || 0x00 != _Head[1] || 0x07 != _Head[2]){
		Serial.println("error1");
		Serial.println("HEAD1");
		Serial.println(_Head[0]);
		Serial.println(_Head[1]);
		Serial.println(_Head[3]);
		Serial.println("PARAMS");
	Serial.println(_Params[0]);
	Serial.println(_Params[1]);
	Serial.println(_Params[2]);
	Serial.println(_Params[3]);
	Serial.println(_Params[4]);
	Serial.println(_Params[5]);
	Serial.println(_Params[6]);
		return false;
	}
	if (_AddressHigh != _Params[0] || _AddressLow != _Params[1] || _NetID != _Params[2] || _Speed != _Params[3] || _Packet != _Params[4] || _Channel != _Params[5] || _Options != _Params[6]){
		Serial.println("error2");
		Serial.println("HEAD1");
		Serial.println(_Head[0]);
		Serial.println(_Head[1]);
		Serial.println(_Head[3]);
		Serial.println("PARAMS");
	Serial.println(_Params[0]);
	Serial.println(_Params[1]);
	Serial.println(_Params[2]);
	Serial.println(_Params[3]);
	Serial.println(_Params[4]);
	Serial.println(_Params[5]);
	Serial.println(_Params[6]);
		return false;
	}
	
	return true;
	} else {
	return false;
	}
	
}

/*
	Отправляет текущие (установленные в библиотеке а не записанные в модуль) настройки по радиоканалу.
	Принимающий модуль должен сразу отправить сообщение об успешном получении настроек(используя старые настройки).
	После этого, принимающий модуль применит отправленные ему настройки.
	Примечание: модуль, который примет настройки не применяет новую скорость UART сразу.
	Новая скорость UART будет применена после любой смены режима работы. При этом в режиме конфигурации
	скорость UART останется на уровне 9600, так как в этом режиме она неизменна.

	Пример:
	Если вы хотите перевести оба модуля на 25 канал связи, вам нужно:
	1. Задать новый канал связи методом setChannel(25)
	2. Отправить новые настройки по радио методом writeSettingsWireless(PERMANENT или TEMPORARY)
	3. В случае успешного выполнения П.2 записать навые настройки в свой модуль методом writeSettings(PERMANENT или TEMPORARY)

	Помните, что остальные настройки удаленного модуля также будут перезаписаны текущими значениями из библиотеки.
	Вы можете сначала считать текущие настройки удаленного модуля в библиотеку методом readSettingsWireless(), изменить только
	некоторые параметры и отправить настройки обратно на удаленный модуль.
	После этого вы можете записать эти настройки в свой модуль или снова загрузить настройки в библиотеку из своего модуля 
	методом readSettings().
*/

bool EBYTE22::writeSettingsWireless(uint8_t memory) {
	
	completeTask(MAX_WAIT_TIME);
	if(!getBusy()){
		
	uint8_t currentMode = getMode();
	setMode(MODE_CONFIG);
	completeTask(1000);
	
	clearBuffer();

	delay(5);

	_s->write((uint8_t)0xCF);
	
	_s->write((uint8_t)0xCF);
	
	_s->write(memory);
	
	_s->write((uint8_t)0x00);
	
	_s->write((uint8_t)0x07);

	_s->write(_AddressHigh);

	_s->write(_AddressLow);

	_s->write(_NetID);

	_s->write(_Speed);

	_s->write(_Packet);
	
	_s->write(_Channel);

	_s->write(_Options);

	delay(5);
	completeTask(1000);
	
	_Head[0] = 0;
	_Head[1] = 0;

	_s->setTimeout(5000);
	_s->readBytes((uint8_t*)&_Head, (uint8_t) 2);
	_s->setTimeout(1000);
	if (0xCF != _Head[0] || 0xCF != _Head[1]){
		clearBuffer();
		setMode(currentMode);
	//	Serial.println("HEAD1");
	//	Serial.println(_Head[0]);
	//	Serial.println(_Head[1]);
	delay(2);
	completeTask(1000);
		return false;
	}
	
	_Params[0] = 0;
	_Params[1] = 0;
	_Params[2] = 0;
	_Params[3] = 0;
	_Params[4] = 0;
	_Params[5] = 0;
	_Params[6] = 0;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_Params, (uint8_t) sizeof(_Params));
	/*
	Serial.println("HEAD");
	Serial.println(_Head[0]);
	Serial.println(_Head[1]);
	Serial.println(_Head[2]);
	
	Serial.println("PARAMS");
	Serial.println(_Params[0]);
	Serial.println(_Params[1]);
	Serial.println(_Params[2]);
	Serial.println(_Params[3]);
	Serial.println(_Params[4]);
	Serial.println(_Params[5]);
	Serial.println(_Params[6]);
	*/
	setMode(currentMode);
	completeTask(100);
	
		
		if ((memory != _Head[0] && 0xC1 != _Head[0]) || 0x00 != _Head[1] || 0x07 != _Head[2]){ //_Head[0] по даташиту должен быть "0xC1", но мои модули возвращают то, что было послано в переменной "memory". Ошибка разработчиков.
		Serial.println("HEAD2 Error");
	//	Serial.println(_Head[0]);
	//Serial.println(_Head[1]);
	//Serial.println(_Head[2]);
		
		return false;
	}
	if (_AddressHigh != _Params[0] || _AddressLow != _Params[1] || _NetID != _Params[2] || _Speed != _Params[3] || _Packet != _Params[4] || _Channel != _Params[5] || _Options != _Params[6]){
//		Serial.println("Params Error");
		return false;
	}
	
	return true;
	} else {
//	Serial.println("Busy");
	return false;
	}	
	
}

// Задает криптографический ключ 0-65535, второй аргумент позволяет применить ключ временно или записать в ПЗУ модуля (см. константы в .h файле).
// Ключ отправляется сразу в модуль, вызывать "writeSettings" нет необходимости.

bool EBYTE22::writeCryptKey(uint16_t key, uint8_t memory) {
	
	completeTask(MAX_WAIT_TIME);
	
	if(!getBusy()){
	uint8_t currentMode = getMode();
	setMode(MODE_CONFIG);
	completeTask(1000);
	
	uint8_t cryptH = ((key & 0xFFFF) >> 8);
	uint8_t cryptL = (key & 0xFF);
		
	clearBuffer();
		
	delay(5);

	_s->write(memory);
	
	_s->write((uint8_t)0x07);
	
	_s->write((uint8_t)0x02);

	_s->write(cryptH);

	_s->write(cryptL);

	delay(10);
	completeTask(1000);
	
	_Params[0] = 0;
	_Params[1] = 0;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_Params, (uint8_t) 2);
	
	setMode(currentMode);
	
	if (0xC1 != _Head[0] || 0x07 != _Head[1] || 0x02 != _Head[2]){
		
		return false;
	}
	if (cryptH != _Params[0] || cryptL != _Params[1]){
	
		return false;
	}

	return true;
	} else {
	return false;
	}
	
}

// Отправить криптографический ключ на принимающий модуль, он должен оповестить об успешности операции и применить ключ
// Второй аргумент позволяет применить ключ временно или записать в ПЗУ модуля (см. константы в .h файле).

bool EBYTE22::writeCryptKeyWireless(uint16_t key, uint8_t memory) {

	completeTask(MAX_WAIT_TIME);
	
	if(!getBusy()){
	uint8_t currentMode = getMode();
	setMode(MODE_CONFIG);
	completeTask(1000);
	
	uint8_t cryptH = ((key & 0xFFFF) >> 8);
	uint8_t cryptL = (key & 0xFF);

	clearBuffer();
	delay(5);

	_s->write((uint8_t)0xCF);
	
	_s->write((uint8_t)0xCF);
	
	_s->write(memory);
	
	_s->write((uint8_t)0x07);
	
	_s->write((uint8_t)0x02);

	_s->write(cryptH);

	_s->write(cryptL);

	delay(10);
	completeTask(1000);
	
	_Head[0] = 0;
	_Head[1] = 0;
	
	_s->setTimeout(4000);
	_s->readBytes((uint8_t*)&_Head, (uint8_t) 2);
	_s->setTimeout(1000);
	if (0xCF != _Head[0] || 0xCF != _Head[1]){
		
	setMode(currentMode);
	clearBuffer();
	/*
		Serial.println("HEAD1");
		Serial.println(_Head[0]);
		Serial.println(_Head[1]);
		*/
	delay(2);
	completeTask(1000);
		return false;
	}
	
	
	_Params[0] = 0;
	_Params[1] = 0;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_Params, (uint8_t) 2);

	setMode(currentMode);
	
	if ((memory != _Head[0] && 0xC1 != _Head[0]) || 0x07 != _Head[1] || 0x02 != _Head[2]){
		
		return false;
	}
	if (cryptH != _Params[0] || cryptL != _Params[1]){
	
		return false;
	}
	
	return true;
	} else {
	return false;
	}
	
}

// Запрашивает настройки по радиоканалу у удаленного модуля, в случае успешного получения, настройки сохраняются в библиотеку, заменяя
// значения, полученные от вашего модуля.

bool EBYTE22::readSettingsWireless() {

	completeTask(MAX_WAIT_TIME);
	
	if(!getBusy()){
		
	uint8_t currentMode = getMode();
	setMode(MODE_CONFIG);
		
	clearBuffer();

	delay(5);

	_s->write((uint8_t)0xCF);
	
	_s->write((uint8_t)0xCF);
	
	_s->write((uint8_t)0xC1);
	
	_s->write((uint8_t)0x00);
	
	_s->write((uint8_t)0x07);

	delay(10);
	completeTask(1000);
	
	_Head[0] = 0;
	_Head[1] = 0;
	_s->setTimeout(5000);
	_s->readBytes((uint8_t*)&_Head, (uint8_t) 2);
	_s->setTimeout(1000);
	if (0xCF != _Head[0] || 0xCF != _Head[1]){
		
	setMode(currentMode);
	
		Serial.println("HEAD1");
		Serial.println(_Head[0]);
		Serial.println(_Head[1]);
		
	delay(2);
	completeTask(1000);
		return false;
	}
	
	_Params[0] = 0;
	_Params[1] = 0;
	_Params[2] = 0;
	_Params[3] = 0;
	_Params[4] = 0;
	_Params[5] = 0;
	_Params[6] = 0;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_Params, (uint8_t) sizeof(_Params));
	
	/*
	Serial.println("HEAD");
	Serial.println(_Head[0]);
	Serial.println(_Head[1]);
	Serial.println(_Head[2]);
	
	Serial.println("PARAMS");
	Serial.println(_Params[0]);
	Serial.println(_Params[1]);
	Serial.println(_Params[2]);
	Serial.println(_Params[3]);
	Serial.println(_Params[4]);
	Serial.println(_Params[5]);
	Serial.println(_Params[6]);
	*/
	
	setMode(currentMode);
	completeTask(100);
	
		
		if (0xC1 != _Head[0] || 0x00 != _Head[1] || 0x07 != _Head[2]){ 
		Serial.println("HEAD2 Error");
		Serial.println(_Head[0]);
	Serial.println(_Head[1]);
	Serial.println(_Head[2]);
		
		return false;
	}
	
	/*
	if (_AddressHigh != _Params[0] || _AddressLow != _Params[1] || _NetID != _Params[2] || _Speed != _Params[3] || _Packet != _Params[4] || _Channel != _Params[5] || _Options != _Params[6]){
		Serial.println("Params Error");
		return false;
	}
	*/
	
	_AddressHigh = _Params[0];
	_AddressLow = _Params[1];
	_NetID = _Params[2];
	_Speed = _Params[3];
	_Packet = _Params[4];
	_Channel = _Params[5];
	_Options = _Params[6];

	_Address =  (_AddressHigh << 8) | (_AddressLow);
	
	_UARTDataRate = (_Speed & 0XE0) >> 5;
	_ParityBit = (_Speed & 0X18) >> 3;
	_AirDataRate = (_Speed & 0X07);
	
	_PacketLength = (_Packet & 0XC0) >> 6;
	_RSSIAmbient = (_Packet & 0X20) >> 5;
	_Power = (_Packet & 0X03);
	
	_RSSIInPacket  = (_Options & 0X80) >> 7;
	_TXMode = (_Options & 0X40) >> 6;
	_Repeater = (_Options & 0X20) >> 5;
	_LBT = (_Options & 0X10) >> 4;
	_WOR = (_Options & 0X08) >> 3;
	_WORCycle = (_Options & 0X07);
	
	
	return true;
	} else {
	Serial.println("Busy");
	return false;
	}	
	
}


// Считать настройки с модуля в библиотеку (значения настроек библиотеки перезапишутся текущими настройками модуля).
// Если в модуль были записаны временные настройки, считаются именно они (если на модуль не прекращалась подача питания).
// Этот метод выполняется автоматически при инициализации.

bool EBYTE22::readSettings() {
	
	completeTask(MAX_WAIT_TIME);
	
	if(!getBusy()){
	uint8_t currentMode = getMode();
	_Params[0] = 0;
	_Params[1] = 0;
	_Params[2] = 0;
	_Params[3] = 0;
	_Params[4] = 0;
	_Params[5] = 0;
	_Params[6] = 0;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	
	setMode(MODE_CONFIG);
	completeTask(3000);

	clearBuffer();

	delay(5);

	_s->write((uint8_t)0xC1);

	_s->write((uint8_t)0x00);

	_s->write((uint8_t)0x07);

	//delay(15);

	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_Params, (uint8_t) sizeof(_Params));

	if (0xC1 != _Head[0] || 0x00 != _Head[1] || 0x07 != _Head[2]){
		Serial.println("HEAD1");
		Serial.println(_Head[0]);
		Serial.println(_Head[1]);
		Serial.println(_Head[2]);
		setMode(currentMode);
		completeTask(100);
		return false;
	}
	
	_AddressHigh = _Params[0];
	_AddressLow = _Params[1];
	_NetID = _Params[2];
	_Speed = _Params[3];
	_Packet = _Params[4];
	_Channel = _Params[5];
	_Options = _Params[6];

	_Address =  (_AddressHigh << 8) | (_AddressLow);
	
	_UARTDataRate = (_Speed & 0XE0) >> 5;
	_ParityBit = (_Speed & 0X18) >> 3;
	_AirDataRate = (_Speed & 0X07);
	
	_PacketLength = (_Packet & 0XC0) >> 6;
	_RSSIAmbient = (_Packet & 0X20) >> 5;
	_Power = (_Packet & 0X03);
	
	_RSSIInPacket  = (_Options & 0X80) >> 7;
	_TXMode = (_Options & 0X40) >> 6;
	_Repeater = (_Options & 0X20) >> 5;
	_LBT = (_Options & 0X10) >> 4;
	_WOR = (_Options & 0X08) >> 3;
	_WORCycle = (_Options & 0X07);

	setMode(currentMode);
	completeTask(100);
	return true;
	} else {
//	Serial.println("Busy");
	return false;
	}	
}

bool EBYTE22::writeWORReceiverDelay(uint16_t time, uint8_t memory) {
	
	
	completeTask(MAX_WAIT_TIME);
	
	if(!getBusy()){
	uint8_t currentMode = getMode();
	uint8_t delayH = ((time & 0xFFFF) >> 8);
	uint8_t delayL = (time & 0xFF);
	
	setMode(MODE_CONFIG);
	completeTask(1000);

	clearBuffer();	
	
	delay(5);

	_s->write(memory);
	
	_s->write((uint8_t)0x09);
	
	_s->write((uint8_t)0x02);

	_s->write(delayH);

	_s->write(delayL);

	delay(10);
	completeTask(1000);
	
	_Params[0] = 0;
	_Params[1] = 0;
	
	_Head[0] = 0;
	_Head[1] = 0;
	_Head[2] = 0;
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));
	_s->readBytes((uint8_t*)&_Params, 2);
	setMode(currentMode);

	if (0xC1 != _Head[0] || 0x09 != _Head[1] || 0x02 != _Head[2]){
		Serial.println("error1");
		Serial.println("HEAD1");
		Serial.println(_Head[0]);
		Serial.println(_Head[1]);
		Serial.println(_Head[3]);
		Serial.println("PARAMS");
	Serial.println(_Params[0]);
	Serial.println(_Params[1]);
	Serial.println(_Params[2]);
	Serial.println(_Params[3]);
	Serial.println(_Params[4]);
	Serial.println(_Params[5]);
	Serial.println(_Params[6]);
		return false;
	}
	if (delayH != _Params[0] || delayL != _Params[1]){
		Serial.println("error2");
		Serial.println("HEAD1");
		Serial.println(_Head[0]);
		Serial.println(_Head[1]);
		Serial.println(_Head[3]);
		Serial.println("PARAMS");
	Serial.println(_Params[0]);
	Serial.println(_Params[1]);
		return false;
	}
	return true;
	} else {
	return false;
	}
	
}





//	В качестве аргумента нужно передать массив типа byte длиной 7, в который будет записана некая информация о модуле. 
//	Что эта информация значит, я не знаю.
		
bool EBYTE22::readProductInfo(byte* infoPrt) {
	
	completeTask(MAX_WAIT_TIME);
	
	if(!getBusy()){
		
	uint8_t currentMode = getMode();
	_infoPrt = infoPrt;

	_infoPrt[0] = 5;
	_infoPrt[1] = 0;
	_infoPrt[2] = 0;
	_infoPrt[3] = 0;
	_infoPrt[4] = 0;
	_infoPrt[5] = 0;
	_infoPrt[6] = 3;
	

	completeTask(1000);
	setMode(MODE_CONFIG);

	clearBuffer();
	
	delay(5);
	
	_Head[0] = 0xC1;
	_Head[1] = 0x80;
	_Head[2] = 0x07;

	_s->write((uint8_t*)&_Head, (uint8_t) 3);
	delay(10);
	
	_s->readBytes((uint8_t*)&_Head, (uint8_t) sizeof(_Head));

	if (0xC1 != _Head[0] || 0x80 != _Head[1] || 0x07 != _Head[2]){
		
		setMode(currentMode);
		completeTask(100);
		return false;
	}
	_s->readBytes((uint8_t*)_infoPrt, 7);
	
	setMode(currentMode);
	completeTask(100);
	return true;
	} else {
	return false;
	}
}

// Очищает буфер, если там остались непрочитанные данные.

void EBYTE22::clearBuffer(){

	byte b;

	while(_s->available()) {
	b = _s->read();
	//_s->readBytes((uint8_t*)&_buf, (uint8_t) 1);
	Serial.print("Clear");
	Serial.print(b);
	}

}
