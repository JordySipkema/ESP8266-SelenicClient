#define _NOMINMAX
#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>

#include <list>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include "Actuator.h"
#include "Config.h"
#include "Log.h"
#include "Timer.h"

extern "C" {
#include "user_interface.h"
}

// #define USE_NEOPIXEL
struct Settings
{
	int id = -1;
	String name;
	std::list<Actuator*> actuators;
} settings;

int getId() { return settings.id; };

Log logger;
Timer timer;

bool callApi(String api, String method, String postData, std::function<void(JsonObject& data)> callback);
bool callApi(String api, String method, JsonObject& postData, std::function<void(JsonObject& data)> callback);
bool callApi(String api, String method, JsonArray& postData, std::function<void(JsonObject& data)> callback);
void sendHeartbeat(void);

// Values are defined in Config.h
const char* ssid = CONFIG_SSID;
const char* password = CONFIG_PASSWORD;
unsigned long endOfSetupTime = 0;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

#ifdef USE_NEOPIXEL
Adafruit_NeoPixel pixels(3, D8, NEO_GRB + NEO_KHZ800);
#endif

void setup() {
#ifdef USE_NEOPIXEL
	// Init the NeoPixels for debugging purposes.
	pixels.begin();
	for (int i = 0; i < 3; i++)
		pixels.setPixelColor(i, pixels.Color(255, 0, 0));
	pixels.show();
#endif

	Serial.begin(115200);
	logger.println();
	logger.println("ESP\tBooting ESP...");
	logger.print("ESP\tChip identifier: ");
	logger.println(ESP.getChipId(), HEX);

	logger.print("WIFI\tConnecting (");
	logger.print(ssid); logger.print(")");
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		logger.print(".");
		WiFi.begin(ssid, password);
		delay(1000);
	}
	logger.println(); logger.println();
	logger.print("WIFI\tConnected to ");
	logger.println(ssid);
	logger.print("WIFI\tIP address: ");
	logger.println(WiFi.localIP());
	logger.print("WIFI\tDNS address: ");
	logger.println(WiFi.dnsIP(0));

	logger.println("API\tConnecting to API for ID");
#ifdef USE_NEOPIXEL
		pixels.setPixelColor(2, pixels.Color(0, 255, 0));
		pixels.show();
#endif
  if(!callApi(String("nodes/") + String(ESP.getChipId(), HEX), "GET", "", [](JsonObject& ret)
  {
    settings.name = ret["data"]["name"].as<const char*>();
    settings.id = ret["data"]["node_id"];
    logger.print("API\tNode ID: ");
    logger.println(settings.id);
    logger.print("API\tNode name: ");
    logger.println(settings.name);
    if(settings.id == 0)
    {
      logger.println("ERROR\tOops, cannot have ID 0");
			logger.println("ESP\tPreparing a reboot.");
      delay(1000);
      ESP.restart();
    }
  })) ESP.restart();

#ifdef USE_NEOPIXEL
	pixels.setPixelColor(1, pixels.Color(0, 255, 0));
	pixels.show();
#endif

	String host = String("esp8266-sensor-") + settings.id;

#ifdef USE_NEOPIXEL
		pixels.setPixelColor(0, pixels.Color(0, 255, 0));
		pixels.show();
#endif

	MDNS.begin(host.c_str());
	timer.begin();

	callApi(String("nodes/") + settings.id + String("/sensors"), "GET", "", [](JsonObject& ret){
		logger.printTime();
		logger.print("API\tSensor information received. Number of sensors: ");
		logger.println(ret["data"].size());
		for(int i = 0; i < ret["data"].size(); i++)
		{
			Actuator* actuator = Actuator::build(ret["data"][i]);
			if(actuator)
				settings.actuators.push_back(actuator);
			else
			{
				logger.print("API\tUnknown sensor or actuator. (type: ");
				logger.print((int)ret["data"][i]["type"]);
				logger.println(")");
			}
		}
	});

	httpUpdater.setup(&httpServer);
	httpServer.begin();
	MDNS.addService("http", "tcp", 80);

	logger.printTime(); logger.println();
	logger.printf("HTTP\tHTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host.c_str());

	httpServer.onNotFound([](){
		httpServer.send(404, "text/plain", "Resource not found...");
	});

	httpServer.on("/", [](){
		httpServer.send(200, "application/json", "{\"type\": \"ESP8266\", \"name\": \"" + settings.name + "\"}");
	});

	httpServer.on("/api/actuate", HTTP_POST, [](){
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(httpServer.arg(0));

    bool activated = false;
    for(auto a : settings.actuators){
      logger.print("Actuator "); logger.println(a->id);
      if(a->id == json["id"]){
        a->activate(json);
        activated = true;
      }
    }
    if(!activated){
      logger.printTime();
      logger.print("REQ\tCould not find activator "); logger.println((int)json["id"]);
			httpServer.send(200, "application/json",  "{\"result\":\"NOK\"}");
    } else httpServer.send(200, "application/json",  "{\"result\":\"ok\"}");
  });


 httpServer.on("/api/actuators", [](){
	 StaticJsonBuffer<200> buffer;
	 JsonArray& sensors = buffer.createArray();

	 for(auto s : settings.actuators)
	 {
		 JsonObject& o = buffer.createObject();
		 o["id"] = s->id;
		 sensors.add(o);
	 }

	 char buf[200];
	 sensors.printTo(buf, 200);
	 httpServer.send(200, "application/json",  buf);
 });

 httpServer.on("/api/rbt", [](){
	 httpServer.send(200, "application/json", "{\"result\":\"reboot\"}");
 });

  sendHeartbeat();
	timer.addCallback(120, &sendHeartbeat);

// All done
#ifdef USE_NEOPIXEL
	pixels.clear();
	pixels.setPixelColor(0, pixels.Color(0, 25, 0));
	pixels.show();
#endif
}

void loop(){
	httpServer.handleClient();
	timer.update();
	delay(1);
}

void sendHeartbeat(void){
	StaticJsonBuffer<200> buffer;
	JsonObject& o = buffer.createObject();
	o["heapspace"] = ESP.getFreeHeap();
	callApi(String("nodes/ping/") + settings.id, "POST", o, [](JsonObject &ret) {  });
}

bool callApi(String api, String method, String postData, std::function<void(JsonObject& data)> callback)
{
  HTTPClient http;
  http.begin(String("http://selenic-api.jordysipkema.nl/" + api)); //HTTP
	http.addHeader("Content-Type", "application/json");
  int httpCode = 0;
  if(method == "GET")         httpCode = http.GET();
  else if(method == "POST")   httpCode = http.POST(postData);
  else { logger.printTime(); logger.println("API\tWrong calling method!"); }

	// logger.println("DEBUG\tPostdata: " + postData);

  if(httpCode > 0) {
      if(httpCode != 200)
        Serial.printf("HTTP\tGET... statuscode %d\n", httpCode);
      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          StaticJsonBuffer<2000> jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject(payload);
          if (!root.success())
          {
            Serial.println("parseObject() failed");
            Serial.println(payload);
            return false;
          }
          callback(root);
      }
      return true;
  } else {
      Serial.printf("HTTP\tGET... failed, error %i: %s\n", httpCode, http.errorToString(httpCode).c_str());
      return false;
  }
}

bool callApi(String api, String method, JsonObject& postData, std::function<void(JsonObject& data)> callback)
{
  char buf[200];
  postData.printTo(buf, 200);
  return callApi(api, method, buf, callback);
}

bool callApi(String api, String method, JsonArray& postData, std::function<void(JsonObject& data)> callback)
{
	char buf[200];
	postData.printTo(buf, 200);
	return callApi(api, method, buf, callback);
}
