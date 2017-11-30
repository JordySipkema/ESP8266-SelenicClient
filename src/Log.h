#pragma once
#include <time.h>
#include <HardwareSerial.h>

class Log : public Print
{
	int currentType;

public:
	int index = 0;
	String lines[100];


	void begin()
	{

	}


	size_t write(uint8_t data)
	{
		Serial.write(data);
	}

	size_t write(const uint8_t *buffer, size_t size)
	{
		Serial.write(buffer, size);
	}

	void printTime()
	{
		time_t now = time(nullptr);

		char* timestr = asctime(localtime(&now));
		timestr[strlen(timestr) - 1] = 0;
		print(timestr);
		print("\t");
	}

	void setType(int type)
	{
		currentType = type;
	}


};


extern Log logger;
