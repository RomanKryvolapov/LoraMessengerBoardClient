 
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
*/

/* 
  Библиотека служит для работы с радиомодулями модели lora компании EBYTE.
  Эти радиомодули используют LoRa модуляцию для организации дальней связи.
  Библиотека написана автором YouTube канала TryTech  http://www.youtube.com/c/TryTech

  Данная библиотека была основана на библиотеке EBYTE от KrisKasprzak 
  Ссылка на источник библиотеки https://github.com/KrisKasprzak/EBYTE
  В отличии от предыдущих моделей, модули lora имеют больший функционал,
  а взаимодействии с ними сильно отличается, поэтому многие методы были
  полностью переписаны и добавлены новые. Но все же я благодарен автору
  библиотеки указанной выше, благодаря ему я сэкономил время.
   
  Версия		Дата			Автор			Описание
  1.0			3/6/2019		TryTech			Создание библиотеки

 Подключение модуля
  Модуль	Контроллер				Описание
  MO		Любой цифровой пин		Пин для управления режимом работы
  M1		Любой цифровой пин		Пин для управления режимом работы
  Rx		Любой цифровой пин		Пин подключается к TX пину микроконтроллера (передача данных от контроллера к модулю)
  Tx		Любой цифровой пин		Пин подключается к RX пину микроконтроллера (передача данных от модуля к контроллеру)
  AUX		Любой цифровой пин		Пин для индикации завершения операции (низкий уровень - модуль занят, высокий - свободен)
  Vcc		3.3 - 5.5В				Для достижения максимальной мощности передатчика, питание должно быть не ниже 5В.
  GND		Земля					При раздельном питании, земля у модуля и микроконтроллера должна быть общей		
 
 Использование библиотеки:
  1. Создать объект последовательного соединения.
  2. Создать объект EBYTE22 с ссылкой на объект последовательного соединения.
  3. Начать последовательное соединение методом "begin"
  4. Инициализировать объект EBYTE22 методом "init"
  5. Сконцигурировать модуль (опционально)
  6. Отправлять и получать данные
  
*/


#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/* 

Задержка (мс) после смены режима, например, с нормального в режим конфигурации.
Я заметил, что значение ниже 22мс иногда приводит к ошибкам. Это происходит из-за
того, что модуль EBYTE не успевает переключится в новый режим работы.

*/

// Задержки в мс, необходимые модулю для завершения некоторых операций. При обнаружении проблем, можно попробовать увеличить.

#define MODE_RECOVER 25				// Общая задержка при смене режима работы
#define RECOVERY_AFTER_WOR 90		// Дополнительная задержка при переходе из режима WOR в любой другой.
#define RECOVERY_AFTER_SLEEP 850	// Дополнительная задержка при переходе из режима SLEEP (сна) в любой другой.

#define MAX_WAIT_TIME 2000

// Режимы работы модуля

#define MODE_NORMAL 0		// Может передавать и принимать данные
#define MODE_WOR 1			// Перед передачей отправляется преамбула для пробуждения приемника( если настроен как передатчик WOR)
#define MODE_CONFIG 3		// Режим конфигурации (для чтения и записи настроек). Скорость UART только 9600.
#define MODE_SLEEP 2		// Режим сна, потребление энергии минимальное, прием, передача и конфигурация не выполняются.

// При записи настроек или криптографического ключа в модуль (в том числе и удаленно),
// можно применить настройки временно(до потери питания) или сохранить в ПЗУ модуля.

#define PERMANENT 0xC0 		// Применить и сохранить в ПЗУ
#define TEMPORARY 0xC2 		// Применить временно

// Параметры бита четности для UART, в режиме конфигурации всегда используется 8N1.

#define PB_8N1 0b00			// по умолчанию
#define PB_8O1 0b01
#define PB_8E1 0b10

//Скорость передачи данных по UART (бод/сек), в режиме конфигурации всегда используется 9600.

#define UBR_1200 0b000		// 1200 
#define UBR_2400 0b001		// 2400 
#define UBR_4800 0b010		// 4800 
#define UBR_9600 0b011		// 9600  по умолчанию
#define UBR_19200 0b100		// 19200 
#define UBR_38400 0b101		// 38400 
#define UBR_57600 0b110		// 57600 
#define UBR_115200 0b111	// 115200 

//Скорость передачи данных по радиоканалу, два модуля должны иметь одинаковую скорость, иначе они не "поймут" друг друга.

#define ADR_300 0b000		// 300 
#define ADR_1200 0b001		// 1200 
#define ADR_2400 0b010		// 2400  по умолчанию
#define ADR_4800 0b011		// 4800 
#define ADR_9600 0b100		// 9600 
#define ADR_19200 0b101		// 19200 
#define ADR_38400 0b110		// 38400 
#define ADR_62500 0b111		// 62500 

/*
// air data rates (other types of modules)
#define ADR_1K 0b000		// 1k 
#define ADR_2K 0b001		// 2K 
#define ADR_5K 0b010		// 4K 
#define ADR_8K 0b011		// 8K 
#define ADR_10K 0b100		// 10K 
#define ADR_15K 0b101		// 15K 
#define ADR_20K 0b110		// 20K 
#define ADR_25K 0b111		// 25K 
*/

//Длина субпакета.

#define PACKET240 0b00      // по умолчанию
#define PACKET128 0b01
#define PACKET64 0b10
#define PACKET32 0b11

//Включение и выключение функций, связанных с RSSI

#define RSSI_ENABLE 0b1
#define RSSI_DISABLE 0b0

// Константы для функции getRSSI. Функция наботает в нормальном и WOR режиме.

#define RSSI_AMBIENT 0b0		// RSSI окружающей среды (шум)
#define RSSI_LAST_RECEIVE 0b1	// RSSI сигнала, при последнем приеме данных.

// Режим передачи

#define TXM_FIXED 0b1			// Адресная передача (с обязательным указанием адреса и канала).
#define TXM_NORMAL 0b0			// Передача по тому же адресу, на который настроен модуль.

// Функция репитера

#define REPEATER_ENABLE 0b1		// Включить
#define REPEATER_DISABLE 0b0	// Выключить

// Мониторинг канала перед передачей, как я понял, позволяет в некоторой степени предотвратить передачу данных, в то время как передачу производит другой модуль.

#define LBT_ENABLE 0b1			// Включить
#define LBT_DISABLE 0b0			// Выключить

// Настройка режима WOR

#define WOR_TRANSMITTER 0b1		// Передатчик WOR
#define WOR_RECEIVER 0b0		// Приемник WOR

// Цикл WOR, чем больше, тем ниже энергопотребление приемника и выше задержки передачи данных.

#define WOR500 0b000			// 500 ms
#define WOR1000 0b001			// 1000 ms
#define WOR1500 0b010			// 2500 ms
#define WOR2000 0b011			// 2000 ms
#define WOR2500 0b100			// 2500 ms
#define WOR3000 0b101			// 3000 ms
#define WOR3500 0b110			// 3500 ms
#define WOR4000 0b111			// 4000 ms

// Мощность передатчика, изучите законодательство вашей страны, касаемо разрешенной мощности в определенных частотных диапазонах.
// Константы для модулей мощностью 1Вт.


// Для модулей мощностью   30dbm:	   22dbm:
#define TP_MAX 0b00		 // 30dbm 		22dbm
#define TP_HIGH 0b01	 // 27dbm		17dbm
#define TP_MID 0b10		 // 24dbm		13dbm
#define TP_LOW 0b11		 // 21dbm		10dbm


class Stream;

class EBYTE22 {

public:

	EBYTE22(Stream *s, uint8_t PIN_M0 = 4, uint8_t PIN_M1 = 5, uint8_t PIN_AUX = 6);

		// Метод инициализации. Служит для конфигурации выводов микроконтроллера и считывания настроек из модуля в библиотеку.
		// После этого вы можете изменять настройки, но они не будут применены сразу, для отправки настроек в модуль есть соответствующие функии.
		
	bool init();
	
		// В качестве аргумента нужно передать массив типа byte длиной 7, в который будет записана некая информация о модуле. 
		// Что эта информация значит, я не знаю.
		
	bool readProductInfo(byte* infoPrt);
	
		// Возвращает true - если модуль занят, например, передает по UART данные, полученные от другого модуля.
		// Перед отправкой данных желательно проверять, занят ли модуль.
		
	bool getBusy();
	
		// Методы для установки настроек. ВНИМАНИЕ. Эти методы всего лишь меняют значения настроек в библиотеке, чтобы
		// их применить, нужно отправить их в модуль.
		
	void setMode(uint8_t mode = MODE_NORMAL);
	void setAddress(uint16_t Address = 0);
	void setAddressH(uint8_t AddressH = 0);
	void setAddressL(uint8_t AddressL = 0);
	void setNetID(uint8_t netID);
	void setUARTBaudRate(uint8_t ubr);
	void setParityBit(uint8_t pb);
	void setAirDataRate(uint8_t adr);
	void setPacketLength(uint8_t packet);
	void setRSSIAmbient(uint8_t rssi);
	void setTransmitPower(uint8_t tp);
	void setChannel(uint8_t channel);
	void setRSSIInPacket(uint8_t rssi);
	void setTransmissionMode(uint8_t txm);
	void setRepeater(uint8_t repeater);
	void setLBT(uint8_t lbt);
	void setWOR(uint8_t wor);
	void setWORCycle(uint8_t worCycle);

		// Есть ли непрочитанные данные? Вернет true - если есть.
		
	bool available();
	
		// Ожидать окончание передачи данных по UART от микроконтроллера к модулю.
	
	void flush();
	
		// Методы для получения текущих значений настроек из библиотеки (не из модуля).
		
	uint8_t getMode();
	uint16_t getAddress();
	uint8_t getAddressH();
	uint8_t getAddressL();
	uint8_t getNetID();
	uint8_t getUARTBaudRate();
	uint8_t getParityBit();
	uint8_t getAirDataRate();
	uint8_t getPacketLength();
	uint8_t getRSSIAmbient();
	uint8_t getTransmitPower();
	uint8_t getChannel();
	uint8_t getRSSIInPacket();
	uint8_t getTransmissionMode();
	uint8_t getRepeater();
	uint8_t getLBT();
	uint8_t getWOR();
	uint8_t getWORCycle();
	
	// Получить значение RSSI окружающей среды или последнего приема данных.
	// Работает в нормальном и WOR режиме. Возвращает значение в "попугаях".
	// Производитель говорит, что получить значение в dBm можно так: -RSSI/2
	// Но я заметил, что в таком случае чем лучше сигнал, тем больше это значение
	// уходит в минус, а должно быть наоборот.
	
	uint8_t getRSSI(uint8_t rssi);
		
	// Считать 1 байт из UART
	
	uint8_t getByte();
	
	// Считать структуру данных
	
	bool getStruct(const void *TheStructure, uint16_t size_);
	
	// Отправить 1 байт
	
	void sendByte(uint8_t TheByte);
	
	// Отправить структуру данных
	
	bool sendStruct(const void *TheStructure, uint16_t size_);
	
	/*
	Отправить адрес и канал получателя (работает только в режиме адресной передачи).
	После отправки адреса, данные должны быть отправлены сразу же, без задержки, иначе
	передатчик оправит по адресу пустой пакет и будет ожидать ввода нового адреса.
	Чтобы передать данные на новый адрес, нужно удостовериться что модуль закончил
	работу со старым. Адрес 0xFFFF (65535) служит для широковещательной передачи (для всех
	модулей в этой сети на указанном канале).
	*/
	
	bool sendTarget(uint16_t targetAddres, uint8_t targetChannel,  unsigned long timeout = 0);
	
	// Все измененные настройки не отправляются в модулль сразу, для применения настроек используются следующие методы:
	
	// Записать все значения настроек из библиотеки в модуль, временно или в ПЗУ
	
	bool writeSettings(uint8_t memory = PERMANENT);
	
	// Отправляет теккущие настройки по радиоканалу на другой модуль.
	// Принимающий модуль должен сразу отправить сообщение об успешном получении настроек(используя старые настройки).
	// После этого, принимающий модуль применит отправленные ему настройки.
	
	bool writeSettingsWireless(uint8_t memory = PERMANENT);
	
	// Запрашивает настройки по радиоканалу у другого модуля, в случае успешного получения настройки сохраняются в библиотеку.
	
	bool readSettingsWireless();
	
	// Установить криптографический ключ (значение 0-65535), временно или в ПЗУ.
	
	bool writeCryptKey(uint16_t key, uint8_t memory);
	
	// Отправить криптографический ключ (значение 0-65535) по радиоканалу на другой модуль, временно или в ПЗУ.
	
	bool writeCryptKeyWireless(uint16_t key, uint8_t memory);

	// Считать настройки с модуля в библиотеку (значения настроек библиотеки перезапишутся текущими настройками модуля).
	// Если в модуль были записаны временные настройки, считаются именно они (если на модуль не прекращалась подача питания).
	// Этот метод выполняется автоматически при инициализации.

	bool readSettings();
		
	// Актуально только для приемника, работающего в режиме WOR
	// После приема данных, приемник WOR, в течении некоторого времени
	// имеет возможность отправить ответ, после чего вернется в цикл ожидания маркера.
	// С помощью следующего метода можно установить это время в мс (0 - 65535).
	// Настройка применится сразу же, временно или с сохранением в ПЗУ.
	
	bool writeWORReceiverDelay(uint16_t time, uint8_t memory);
	
	// Если модуль чем - то занят (на пине AUX логический 0), можно подождать когда он освободится с определенный таймаутом.
	
	void completeTask(unsigned long timeout = 0);
	
protected:

	void buildOptionByte();
	void buildPacketByte();
	void buildSpeedByte();
	
private:
	
	void clearBuffer();
	
	Stream*  _s;
	byte* _infoPrt;

	// Переменные пинов
	
	int8_t _M0;
	int8_t _M1;
	int8_t _AUX;

	// Буферы, в которые читаются настройки и заголовки ответов от модуля
	// Иногда используются для записи
	
	uint8_t _Params[7];
	uint8_t _Head[3];

	uint8_t _Speed;
	uint8_t _Packet;
	uint8_t _Options;

	// Переменные настроек
	
	uint8_t _AddressHigh;
	uint8_t _AddressLow;
	uint16_t _Address;
	uint8_t _NetID;
	uint8_t _UARTDataRate;
	uint8_t _ParityBit;
	uint8_t _AirDataRate;
	uint8_t _PacketLength;
	uint8_t _RSSIAmbient;
	uint8_t _Power;
	uint8_t _Channel;
	uint8_t _RSSIInPacket;
	uint8_t _TXMode;
	uint8_t _Repeater;
	uint8_t _LBT;
	uint8_t _WOR;
	uint8_t _WORCycle;

	uint8_t _buf;
	uint8_t _mode;

};

