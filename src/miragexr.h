#ifndef Mirage_h
#define Mirage_h

#include "Arduino.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

#define PINS_COUNT 24
#define PWM_MAX 255 // resolution of PWM

enum class ServerMethods
{
  SET_DIGITAL_PIN,
};

enum class ClientMethods
{
  RECEIVE_BROADCAST = 0,
  SET_PIN_COUNT = 1,
  SET_MAX_PWM = 2,
};

enum class ClientCallbacks
{
  SET_PEN_DONE
};

struct VPin
{
  uint8_t index;
  uint8_t ioMode;
  uint8_t digitalValue;
  uint16_t analogValue;
  String textValue;     // new field for string values
  uint16_t activeValueType; // 0: digital, 1: analog, 2: text
};

static VPin *pins = new VPin[PINS_COUNT];
static bool isInitialized = false;
static unsigned long lastBroadcastTime = 0;
static const unsigned long broadcastInterval = 300;

inline void vPinMode(int index, int mode)
{
  Serial.println("vPinMode called with index: " + String(index) + ", mode: " + String(mode));
  pins[index].ioMode = mode;
  pins[index].digitalValue = LOW;
  pins[index].analogValue = 0;
  pins[index].textValue = "";    // initialize empty string
  if (mode == INPUT || mode == INPUT_PULLUP || mode == OUTPUT)
  {
    pins[index].activeValueType = 0; // digital
  }
  else
  {
    pins[index].activeValueType = 1; // analog
  }
}

inline void vDigitalWrite(int index, int value)
{
  if (pins[index].ioMode == OUTPUT)
  {
    pins[index].digitalValue = value;
  }
}

inline int vDigitalRead(int index)
{
  return pins[index].digitalValue;
}

inline void vAnalogWrite(int index, int value)
{
  if (pins[index].ioMode == OUTPUT)
  {
    pins[index].analogValue = value;
    pins[index].activeValueType = 1; // mark as analog
  }
}

inline int vAnalogRead(int index)
{
  pins[index].activeValueType = 1; // mark as analog
  return pins[index].analogValue;
}

inline void vTextWrite(int index, String value)
{
  if (pins[index].ioMode == OUTPUT)
  {
    pins[index].textValue = value;
    pins[index].activeValueType = 2; // mark as text
  }
}

inline String vTextRead(int index)
{
  pins[index].activeValueType = 2; // mark as text
  return pins[index].textValue;
}

inline void initPins()
{
  for (int i = 0; i < PINS_COUNT; i++)
  {
    pins[i].index = i;
    pins[i].ioMode = INPUT;
    pins[i].digitalValue = LOW;
    pins[i].analogValue = 0;
    pins[i].textValue = "";    // initialize empty string
    pins[i].activeValueType = 0;
  }
}

class MirageXR
{
private:
  char *_ssid;
  char *_password;
  int _port;
  WebSocketsServer _webSocket;

  inline void invokeOnClient(int method, const String &data)
  {
    DynamicJsonDocument message(2248);
    message["method"] = method;
    message["data"] = data;
    String output;
    serializeJson(message, output);
    _webSocket.broadcastTXT(output);
  }

  inline void connectToWifi()
  {
    WiFi.begin(_ssid, _password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  inline void setPinCountOnClient()
  {
    invokeOnClient((int)ClientMethods::SET_PIN_COUNT, String(PINS_COUNT));
  }

  inline void setMaxPWMOnClient()
  {
    invokeOnClient((int)ClientMethods::SET_MAX_PWM, String(PWM_MAX));
  }

  inline void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
  {
    switch (type)
    {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
    {
      IPAddress ip = _webSocket.remoteIP(num);
      Serial.printf("[%u] Connection from ", num);
      Serial.println(ip.toString());
      setPinCountOnClient();
      setMaxPWMOnClient();
    }
    break;
    case WStype_TEXT:
      Serial.printf("[%u] Text: %s\n", num, payload);
      handleClientRequest((char *)payload);
      break;
    default:
      break;
    }
  }

  inline void broadcastPinData()
  { // Fixed typo: "brodcast" -> "broadcast"
    unsigned long currentTime = millis();
    if (currentTime - lastBroadcastTime >= broadcastInterval)
    {
      DynamicJsonDocument doc(4096); // increased size to accommodate strings
      JsonArray pinArray = doc.to<JsonArray>();

      for (int i = 0; i < PINS_COUNT; i++)
      {
        JsonObject pin = pinArray.createNestedObject();
        pin["pinIndex"] = i;
        pin["ioMode"] = pins[i].ioMode;
        pin["digitalValue"] = pins[i].digitalValue;
        pin["analogValue"] = pins[i].analogValue;
        pin["textValue"] = pins[i].textValue;    // add text value to broadcast
        pin["activeValueType"] = pins[i].activeValueType;
      }

      String output;
      serializeJson(doc, output);
      invokeOnClient((int)ClientMethods::RECEIVE_BROADCAST, output); // Fixed enum name

      lastBroadcastTime = currentTime;
    }
  }

  inline void handleClientRequest(char *jsonRequest)
  {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonRequest);

    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    int method = doc["method"] | -1;
    ServerMethods clientRequest = static_cast<ServerMethods>(method);

    switch (clientRequest)
    {
    case ServerMethods::SET_DIGITAL_PIN:
      Serial.println("setDigitalPin...");
      {
        int pin = doc["args"]["pin"] | -1;
        int value = doc["args"]["value"] | 0;
        if (pin >= 0 && pin < PINS_COUNT)
        {
          vDigitalWrite(pin, value);
        }
      }
      break;
    default:
      Serial.println("Unknown request type");
      break;
    }
  }

public:
  inline MirageXR(char *ssid, char *password, int port)
      : _ssid(ssid), _password(password), _port(port), _webSocket(port)
  {

    if (!isInitialized)
    {
      initPins();
      isInitialized = true;
    }
  }

  inline void start()
  {
    connectToWifi();
    _webSocket.begin();
    _webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
                       { this->onWebSocketEvent(num, type, payload, length); });
  }

  inline void loop()
  {
    _webSocket.loop();
    broadcastPinData();
  }
};

#endif