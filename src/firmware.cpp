#include <Waspmote.h>
#include <Wasp4G.h>
#include <smartWaterIons.h>
#include <ArduinoJson.h>

#define CONCENTRATION_ION_POINT_1 0.5F
#define CONCENTRATION_ION_POINT_2 150.0F
#define CONCENTRATION_ION_POINT_3 2000.0F
#define ION_NO_POINTS 3

#define ION_CALCIUM_VOLTAGE_P1 2.7809031009F
#define ION_CALCIUM_VOLTAGE_P2 3.5055961608F
#define ION_CALCIUM_VOLTAGE_P3 3.7F

#define ION_NITRATE_VOLTAGE_P1 3.6902215480F
#define ION_NITRATE_VOLTAGE_P2 3.6181249618F
#define ION_NITRATE_VOLTAGE_P3 3.7F

#define ION_POTASSIUM_VOLTAGE_P1 3.1926817893F
#define ION_POTASSIUM_VOLTAGE_P2 3.5151250362F
#define ION_POTASSIUM_VOLTAGE_P3 3.7F

#define NO_ION_SENSORS 3

#define ION_STATION_CODE "ION001"

#define _4G_APN_HOST "internet.itelcel.com"
#define _4G_APN_USER "webgprs"
#define _4G_APN_PASS "webgprs2002"

#define SERVER_HOST "clustervalley.agricos.mx"
#define SERVER_PORT 80
#define SERVER_RESOURCE "/api/Measure"

// Global static resource for output data
char http_data[512];

#define CONCENTRATION_CALCULATION_MINUTES 8
#define SECONDS_TO_MILIS(milis) (milis * 1000.0f)
#define MINUTES_TO_SECONDS(min) (min * 60.0f)

#define MINUTES_TO_MILLIS(min) (SECONDS_TO_MILIS(MINUTES_TO_SECONDS(min)))

void configure();
void updateTime();
void updateIonsConcentration();
void sendDataToServer();
void getTimeFrom4G();
void awaitTimeBackground(long _delay, void (*process)(void));
void ionsProcessFunc();
void addMeasureToArray(JsonArray &arr, float _measure, const __FlashStringHelper *code);
void buildMeasuresJson();
void readMeasures();

typedef enum
{
  ION_SOCKET_A = SOCKETA,
  ION_SOCKET_B = SOCKETB,
  ION_SOCKET_C = SOCKETC,
  ION_SOCKET_D = SOCKETD,
} IonSocket_e;

class GenericIonSensor
{
private:
  ionSensorClass internal;
  IonSocket_e _socket;
  float internalMeasure;

public:
  GenericIonSensor(IonSocket_e socket, float v_points[], float c_points[], uint8_t noPoints)
      : internal(socket), _socket(socket)
  {
    internal.setCalibrationPoints(v_points, c_points, noPoints);
  }
  float read() { return internal.read(); }
  float calculateConcentration()
  {
    internalMeasure = internal.calculateConcentration(read());
    return calculateConcentration();
  }
  float concentration() const
  {
    return internalMeasure;
  }
};

class BatteryInfo
{
public:
  uint8_t getChargePercent() { return PWR.getBatteryLevel(); }
  float getVoltage() { return PWR.getBatteryVolts(); }
  float getCurrent() { return PWR.getBatteryCurrent(); }
  bool isCharging() { return PWR.getChargingState(); }

  void printStatus()
  {
    USB.print(F("    Battery: [ "));
    USB.print(getChargePercent());
    USB.print(F("%, "));
    USB.print(getVoltage());
    USB.print(F("V, "));
    USB.print(getCurrent());
    USB.print(F("mA, "));
    USB.print(isCharging() ? F(" charging") : F(" discharging"));
    USB.println(F("]"));
  }
};

class TemperatureInfo
{
private:
  pt1000Class tempSensor;

public:
  float read() { return tempSensor.read(); }

  void printStatus()
  {
    USB.print(F("    Temperature: "));
    USB.print(read());
    USB.println(F("°C"));
  }
};

struct IonMeasures
{
private:
  void printConcentration(const __FlashStringHelper *name, float concentration)
  {
    USB.print(name);
    USB.print(concentration);
    USB.println(F("ppm"));
  }

public:
  float calciumConcentration;
  float nitrateConcentration;
  float potassiumConcentration;
  float temperature;
  uint8_t batteryLevel;

  void addMeasuresToJson(JsonArray &measures)
  {
    addMeasureToArray(measures, calciumConcentration, F("cCa"));
    addMeasureToArray(measures, nitrateConcentration, F("cNo3"));
    addMeasureToArray(measures, potassiumConcentration, F("cK"));
    addMeasureToArray(measures, temperature, F("ion_temp"));
    addMeasureToArray(measures, batteryLevel, F("ion_bl"));
  }

  void serializeToUSB()
  {
    USB.println(F(" Ion measures -----------------------------------"));
    printConcentration(F("       Calcium: "), calciumConcentration);
    printConcentration(F("       Nitrate: "), nitrateConcentration);
    printConcentration(F("     Potassium: "), potassiumConcentration);
  }
};

float ionConcentrationPoints[ION_NO_POINTS] = {
    CONCENTRATION_ION_POINT_1,
    CONCENTRATION_ION_POINT_2,
    CONCENTRATION_ION_POINT_3};
float ionCalciumVoltage[ION_NO_POINTS] = {
    ION_CALCIUM_VOLTAGE_P1,
    ION_CALCIUM_VOLTAGE_P2,
    ION_CALCIUM_VOLTAGE_P3};
float ionNitrateVoltage[ION_NO_POINTS] = {
    ION_NITRATE_VOLTAGE_P1,
    ION_NITRATE_VOLTAGE_P2,
    ION_NITRATE_VOLTAGE_P3};
float ionPotassiumVoltage[ION_NO_POINTS] = {
    ION_POTASSIUM_VOLTAGE_P1,
    ION_POTASSIUM_VOLTAGE_P2,
    ION_POTASSIUM_VOLTAGE_P3};

GenericIonSensor calciumSensor(ION_SOCKET_A, ionCalciumVoltage, ionConcentrationPoints, ION_NO_POINTS);
GenericIonSensor nitrateSensor(ION_SOCKET_B, ionNitrateVoltage, ionConcentrationPoints, ION_NO_POINTS);
GenericIonSensor potassiumSensor(ION_SOCKET_C, ionPotassiumVoltage, ionConcentrationPoints, ION_NO_POINTS);

GenericIonSensor ionSensorsBus[NO_ION_SENSORS] = {calciumSensor, nitrateSensor, potassiumSensor};

StaticJsonDocument<1024> jsonDocument;
BatteryInfo Battery;
TemperatureInfo Temperature;
timestamp_t Time;
char timeString[50];
IonMeasures measures;

void setup()
{
  configure();
  awaitTimeBackground(MINUTES_TO_MILLIS(CONCENTRATION_CALCULATION_MINUTES), ionsProcessFunc);
  buildMeasuresJson();
  sendDataToServer();
  PWR.deepSleep("31:00:00:00", RTC_OFFSET, RTC_ALM1_MODE1, ALL_OFF);
}

void loop() {}

void configure()
{
  USB.ON();
  RTC.ON();
  _4G.ON();
  _4G.set_APN(_4G_APN_HOST, _4G_APN_USER, _4G_APN_PASS);
  SWIonsBoard.ON();
  pinMode(DIGITAL8, OUTPUT);
  digitalWrite(DIGITAL8, LOW);
  getTimeFrom4G();
}

void updateTime()
{
  RTC.getTime();
  Time.second = RTC.second;
  Time.minute = RTC.minute;
  Time.hour = RTC.hour;
  Time.day = RTC.day;
  Time.date = RTC.date;
  Time.month = RTC.month;
  Time.year = RTC.year;

  for (size_t i = 0; i < 50; i++) timeString[i] = 0x00;
  sprintf(timeString, "20%i-%02i-%02iT%02i:%02i:%02iZ",
    Time.year, Time.month, Time.day, Time.hour, Time.minute, Time.second);
}

void updateIonsConcentration()
{
  for (uint8_t i = 0; i < NO_ION_SENSORS; i++)
  {
    ionSensorsBus[i].calculateConcentration();
  }
}

void sendDataToServer()
{
  _4G.httpSetContentType("application/json");
  _4G.http(Wasp4G::HTTP_POST, SERVER_HOST, SERVER_PORT, SERVER_RESOURCE, http_data);
}

void getTimeFrom4G()
{
  _4G.setTimeFrom4G();
}

void awaitTimeBackground(long _delay, void (*process)(void))
{
  long start_time = millis();
  while ((millis() - start_time) < _delay)
  {
    process();
  }
}

void ionsProcessFunc()
{
  readMeasures();
}

void addMeasureToArray(JsonArray &arr, float _measure, const __FlashStringHelper *code)
{
  JsonObject measure = arr.createNestedObject();
  measure["m"] = code;
  measure["v"] = _measure;
}

void buildMeasuresJson()
{
  JsonObject dispositivo = jsonDocument.createNestedArray("d").createNestedObject();
  JsonArray mediciones = dispositivo.createNestedArray("m");

  jsonDocument["s"] = timeString;
  dispositivo["k"] = ION_STATION_CODE;
  measures.addMeasuresToJson(mediciones);

  serializeJson(jsonDocument, http_data);
  jsonDocument.clear();
}

void readMeasures()
{
  updateIonsConcentration();

  measures.calciumConcentration = calciumSensor.concentration();
  measures.nitrateConcentration = calciumSensor.concentration();
  measures.potassiumConcentration = calciumSensor.concentration();
  measures.batteryLevel = Battery.getChargePercent();
  measures.temperature = Temperature.read();

  measures.serializeToUSB();
  Battery.printStatus();
  Temperature.printStatus();
}