#include "web.h"
#include <wifi.h>
#include "webfs.h"
#include <led.h>

ESP8266WebServer WEB::server(80);
//holds the current upload
File WEB::fsUploadFile;

void WEB::init()
{
	SPIFFS.begin();
	{
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {
			//String fileName = dir.fileName();
			//size_t fileSize = dir.fileSize();
			//DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
		}
	//DBG_OUTPUT_PORT.printf("\n");
	}


	//SERVER INIT
	server.on("/set", HTTP_GET, []() {
		if (server.hasArg("color"))
		{
			uint32_t color = server.arg("color").toInt();
			LED::setColor(color);
		} else if (server.hasArg("color_r") && server.hasArg("color_g") && server.hasArg("color_b"))
		{
			uint8_t r = server.arg("color_r").toInt();
			uint8_t g = server.arg("color_g").toInt();
			uint8_t b = server.arg("color_b").toInt();
			LED::setColor(r, g, b);
		}

		if (server.hasArg("brightness"))
		{
			uint8_t brightness = server.arg("brightness").toInt();
			LED::setBrightness(brightness);
		}

		if (server.hasArg("mode"))
		{
			uint8_t mode = server.arg("mode").toInt();
			LED::setMode(mode);
		}
		
		if (server.hasArg("speed"))
		{
			uint16_t speed = server.arg("speed").toInt();
			LED::setSpeed(speed);
		}
		
		server.send(200, "text/json", "OK");
	});

	server.on("/get", HTTP_GET, []() {
		uint32_t c = LED::getColor();
		uint8_t r = (uint8_t)(c >> 16);
		uint8_t g = (uint8_t)(c >> 8);
		uint8_t b = (uint8_t)(c >> 0);
		
		String json = "{";
		json += "\"color\": {";
		json += 	"\"c\": " + String(c);
		json += 	", \"r\": " + String(r);
		json += 	", \"g\": " + String(g);
		json += 	", \"b\": " + String(b);
		json += "}";
		json += ", \"brightness\":" + String(LED::getBrightness());
		json += ", \"mode\":" + String(LED::getMode());
		json += ", \"speed\":" + String(LED::getSpeed());
		json += "}";
		server.send(200, "text/json", json);
		json = String();
	});

	//list directory
	server.on("/list", HTTP_GET, handleFileListOld);
	server.on("/listnew", HTTP_GET, handleFileList);
	//load editor
	server.on("/edit", HTTP_GET, []() {
	if (!handleFileRead("/edit.htm")) {
		server.send(404, "text/plain", "FileNotFound");
	}
	});
	//create file
	server.on("/edit", HTTP_PUT, handleFileCreate);
	//delete file
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	//first callback is called after the request has ended with all parsed arguments
	//second callback handles file uploads at that location
	server.on("/edit", HTTP_POST, []() {
		server.send(200, "text/plain", "");
	}, handleFileUpload);

	//called when the url is not defined here
	//use it to load content from SPIFFS
	server.onNotFound([]() {
	if (!handleFileRead(server.uri())) {
		server.send(404, "text/plain", "FileNotFound");
	}
	});

	//get heap status, analog input value and all GPIO statuses in one json call
	server.on("/all", HTTP_GET, []() {
		String json = "{";
		json += "\"heap\":" + String(ESP.getFreeHeap());
		json += ", \"analog\":" + String(analogRead(A0));
		json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
		json += "}";
		server.send(200, "text/json", json);
		json = String();
	});
	server.begin();
	//DBG_OUTPUT_PORT.println("HTTP server started");
}

void WEB::handle()
{
	server.handleClient();
}