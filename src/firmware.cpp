#include <Waspmote.h>
#include <Wasp4G.h>
#include <smartWaterIons.h>
#include <ArduinoJson.h>

#define PYTHON_GRAPH_OUT_ENABLE false

#define ION_NO_POINTS 3
#define ION_NO_MEASURES 4

#define CONCENTRATION_ION_POINT_1 0.5F
#define CONCENTRATION_ION_POINT_2 150.0F
#define CONCENTRATION_ION_POINT_3 2000.0F

// First measure of the day
#define ION_CALCIUM_VOLTAGE_P1_M1 2.7809031009F
#define ION_CALCIUM_VOLTAGE_P2_M1 3.5055961608F
#define ION_CALCIUM_VOLTAGE_P3_M1 3.7F

#define ION_NITRATE_VOLTAGE_P1_M1 3.6902215480F
#define ION_NITRATE_VOLTAGE_P2_M1 3.6181249618F
#define ION_NITRATE_VOLTAGE_P3_M1 3.7F

#define ION_POTASSIUM_VOLTAGE_P1_M1 3.1926817893F
#define ION_POTASSIUM_VOLTAGE_P2_M1 3.5151250362F
#define ION_POTASSIUM_VOLTAGE_P3_M1 3.7F

// Second measure of the day
#define ION_CALCIUM_VOLTAGE_P1_M2 2.7809031009F
#define ION_CALCIUM_VOLTAGE_P2_M2 3.5055961608F
#define ION_CALCIUM_VOLTAGE_P3_M2 3.7F

#define ION_NITRATE_VOLTAGE_P1_M2 3.6902215480F
#define ION_NITRATE_VOLTAGE_P2_M2 3.6181249618F
#define ION_NITRATE_VOLTAGE_P3_M2 3.7F

#define ION_POTASSIUM_VOLTAGE_P1_M2 3.1926817893F
#define ION_POTASSIUM_VOLTAGE_P2_M2 3.5151250362F
#define ION_POTASSIUM_VOLTAGE_P3_M2 3.7F

// Third measure of the day
#define ION_CALCIUM_VOLTAGE_P1_M3 2.7809031009F
#define ION_CALCIUM_VOLTAGE_P2_M3 3.5055961608F
#define ION_CALCIUM_VOLTAGE_P3_M3 3.7F

#define ION_NITRATE_VOLTAGE_P1_M3 3.6902215480F
#define ION_NITRATE_VOLTAGE_P2_M3 3.6181249618F
#define ION_NITRATE_VOLTAGE_P3_M3 3.7F

#define ION_POTASSIUM_VOLTAGE_P1_M3 3.1926817893F
#define ION_POTASSIUM_VOLTAGE_P2_M3 3.5151250362F
#define ION_POTASSIUM_VOLTAGE_P3_M3 3.7F

// Four measure of the day
#define ION_CALCIUM_VOLTAGE_P1_M4 2.7809031009F
#define ION_CALCIUM_VOLTAGE_P2_M4 3.5055961608F
#define ION_CALCIUM_VOLTAGE_P3_M4 3.7F

#define ION_NITRATE_VOLTAGE_P1_M4 3.6902215480F
#define ION_NITRATE_VOLTAGE_P2_M4 3.6181249618F
#define ION_NITRATE_VOLTAGE_P3_M4 3.7F

#define ION_POTASSIUM_VOLTAGE_P1_M4 3.1926817893F
#define ION_POTASSIUM_VOLTAGE_P2_M4 3.5151250362F
#define ION_POTASSIUM_VOLTAGE_P3_M4 3.7F

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

#define CONCENTRATION_CALCULATION_MINUTES 30

#define SECONDS_TO_MILIS(milis) (milis * 1000.0f)
#define MINUTES_TO_SECONDS(min) (min * 60.0f)
#define MINUTES_TO_MILLIS(min) (SECONDS_TO_MILIS(MINUTES_TO_SECONDS(min)))

float ionConcentrationPoints[ION_NO_POINTS] = {
    CONCENTRATION_ION_POINT_1,
    CONCENTRATION_ION_POINT_2,
    CONCENTRATION_ION_POINT_3};

void configure();
void updateTime();
void updateIonsConcentration();
void sendDataToServer();
void getTimeFrom4G();
void awaitTimeBackground(long _delay, void (*process)(long));
bool timeoutFunction(long timeout, bool (*process)(long));
void ionsProcessFunc(long);
void addMeasureToArray(JsonArray &arr, float _measure, const __FlashStringHelper *code);
void buildMeasuresJson();

template <typename Tarray>
void printArray(Tarray *array, uint8_t len);

typedef enum
{
  ION_SOCKET_A = SOCKETA,
  ION_SOCKET_B = SOCKETB,
  ION_SOCKET_C = SOCKETC,
  ION_SOCKET_D = SOCKETD,
} IonSocket_e;

struct MeasuresInDate
{
  uint8_t date;
  uint8_t month;
  uint8_t year;

  uint8_t measures;
};

bool isEqual(MeasuresInDate m1, MeasuresInDate m2)
{
  return m1.year == m2.year && m1.month == m2.month && m1.date == m2.date;
}

void serializeUSB(MeasuresInDate measures)
{
  USB.print(F(" Date: "));
  USB.print(measures.year);
  USB.print(F("/"));
  USB.print(measures.month);
  USB.print(F("/"));
  USB.print(measures.date);
  USB.print(F(" Measures: "));
  USB.print(measures.measures);
}

class IonMeasuresPerDay
{
  const uint8_t eeprom_addr = 0x400;
  MeasuresInDate _measureDate;

  void updateDateFromRTC()
  {
    _measureDate.year = RTC.year;
    _measureDate.month = RTC.month;
    _measureDate.date = RTC.date;
  }

  void saveMeasureDate()
  {
    Utils.writeBlockEEPROM(eeprom_addr, (void *)&_measureDate, sizeof(MeasuresInDate));
  }

  MeasuresInDate readMeasureDate()
  {
    MeasuresInDate storedData;
    Utils.readBlockEEPROM(eeprom_addr, (void *)&storedData, sizeof(MeasuresInDate));
    return storedData;
  }

public:
  uint8_t begin()
  {
    updateDateFromRTC();
    MeasuresInDate storedData = readMeasureDate();
#if !PYTHON_GRAPH_OUT_ENABLE
    USB.print(F(" RTC info: "));
    serializeUSB(_measureDate);
    USB.print(" EEPROM: ");
    serializeUSB(storedData);
    USB.println();
#endif
    if (isEqual(_measureDate, storedData))
    {
      _measureDate.measures = storedData.measures;
      _measureDate.measures++;
    }
    else
    {
      _measureDate.measures = 0;
    }
    saveMeasureDate();
#if !PYTHON_GRAPH_OUT_ENABLE
    USB.print(F(" Result: "));
    serializeUSB(_measureDate);
    USB.println();
#endif
  }

  uint8_t noMeasuresInDay()
  {
    return _measureDate.measures;
  }
};

IonMeasuresPerDay IonUtils;

struct IonCalibrationPoints
{
  float *voltage_points[ION_NO_MEASURES];
  float *c_points;

  IonCalibrationPoints(
      float voltage_points_1[ION_NO_POINTS],
      float voltage_points_2[ION_NO_POINTS],
      float voltage_points_3[ION_NO_POINTS],
      float voltage_points_4[ION_NO_POINTS],
      float _c_points[ION_NO_POINTS])
  {
    c_points = _c_points;
    voltage_points[0] = voltage_points_1;
    voltage_points[1] = voltage_points_2;
    voltage_points[2] = voltage_points_3;
    voltage_points[4] = voltage_points_3;
  }

  float *getPointsByMeasureIndex(uint8_t noMeasure)
  {
    return voltage_points[noMeasure % ION_NO_MEASURES];
  }

  void serializeCalibrationToUSB(const char *ionName)
  {
#if !PYTHON_GRAPH_OUT_ENABLE
    USB.print(F("   Ion "));
    USB.print(ionName);
    USB.print(F(" calibration poins: "));
    printArray(c_points, ION_NO_POINTS);
    USB.println(F(" ppm"));

    for (uint8_t i = 0; i < ION_NO_MEASURES; i++)
    {
      USB.print(F(" Measure "));
      USB.print(i);
      USB.print(F(": "));
      printArray(voltage_points[i], ION_NO_POINTS);
    }
#endif
  }
};

class GenericIonSensor
{
private:
  ionSensorClass internal;
  IonSocket_e _socket;
  float internalMeasure;
  float _voltage;

public:
  GenericIonSensor(IonSocket_e socket, float v_points[], float c_points[], uint8_t noPoints)
      : internal(socket), _socket(socket)
  {
    internal.setCalibrationPoints(v_points, c_points, noPoints);
  }
  float read()
  {
    _voltage = internal.read();
    return _voltage;
  }
  float calculateConcentration()
  {
    float rawLecture = read();
    internalMeasure = internal.calculateConcentration(rawLecture);
    return internalMeasure;
  }
  float concentration() const
  {
    return internalMeasure;
  }
  float voltage() const
  {
    return _voltage;
  }
};

class GenericIonSensorFactory
{
public:
  static GenericIonSensor build(IonSocket_e socket, IonCalibrationPoints points)
  {
    uint8_t noMeasures = IonUtils.noMeasuresInDay();
    GenericIonSensor sensor(socket, points.getPointsByMeasureIndex(noMeasures), ionConcentrationPoints, ION_NO_POINTS);
    return sensor;
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
    USB.print(F("       Battery: ["));
    USB.print(getChargePercent(), DEC);
    USB.print(F("%, "));
    USB.print(getVoltage());
    USB.print(F("V, "));
    USB.print(isCharging() ? F(" charging") : F(" discharging"));
    USB.println(F("]"));
  }
};

class TemperatureInfo
{
private:
  pt1000Class tempSensor;
  float temperature;

public:
  float read()
  {
    temperature = tempSensor.read();
    return temperature;
  }

  void printStatus()
  {
    USB.print(F("   Temperature: "));
    USB.print(temperature);
    USB.println(F("°C"));
  }
};

struct IonMeasures
{
private:
  void printConcentration(const __FlashStringHelper *name, float concentration, float rawVoltage)
  {
    USB.print(name);
    USB.print(concentration);
    USB.print(F("ppm - "));
    USB.print(rawVoltage);
    USB.println(F("mV"));
  }

public:
  float calciumConcentration;
  float nitrateConcentration;
  float potassiumConcentration;
  float temperature;
  uint8_t batteryLevel;

  float calciumVoltage;
  float nitrateVoltage;
  float potassiumVoltage;

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
#if PYTHON_GRAPH_OUT_ENABLE
    USB.print(calciumVoltage);
    USB.print(" ");
    USB.print(nitrateVoltage);
    USB.print(" ");
    USB.println(potassiumVoltage);
#else
    USB.println(F(" Ion measures ---------------------------------------------"));
    printConcentration(F("       Calcium: "), calciumConcentration, calciumVoltage);
    printConcentration(F("       Nitrate: "), nitrateConcentration, nitrateVoltage);
    printConcentration(F("     Potassium: "), potassiumConcentration, potassiumVoltage);
#endif // PYTHON_GRAPH_OUT_ENABLE
  }
};

float ionCalciumVoltage_m1[ION_NO_POINTS] = {
    ION_CALCIUM_VOLTAGE_P1_M1,
    ION_CALCIUM_VOLTAGE_P2_M1,
    ION_CALCIUM_VOLTAGE_P3_M1};
float ionCalciumVoltage_m2[ION_NO_POINTS] = {
    ION_CALCIUM_VOLTAGE_P1_M2,
    ION_CALCIUM_VOLTAGE_P2_M2,
    ION_CALCIUM_VOLTAGE_P3_M2};
float ionCalciumVoltage_m3[ION_NO_POINTS] = {
    ION_CALCIUM_VOLTAGE_P1_M3,
    ION_CALCIUM_VOLTAGE_P2_M3,
    ION_CALCIUM_VOLTAGE_P3_M3};
float ionCalciumVoltage_m4[ION_NO_POINTS] = {
    ION_CALCIUM_VOLTAGE_P1_M4,
    ION_CALCIUM_VOLTAGE_P2_M4,
    ION_CALCIUM_VOLTAGE_P3_M4};

float ionNitrateVoltage_m1[ION_NO_POINTS] = {
    ION_NITRATE_VOLTAGE_P1_M1,
    ION_NITRATE_VOLTAGE_P2_M1,
    ION_NITRATE_VOLTAGE_P3_M1};
float ionNitrateVoltage_m2[ION_NO_POINTS] = {
    ION_NITRATE_VOLTAGE_P1_M2,
    ION_NITRATE_VOLTAGE_P2_M2,
    ION_NITRATE_VOLTAGE_P3_M2};
float ionNitrateVoltage_m3[ION_NO_POINTS] = {
    ION_NITRATE_VOLTAGE_P1_M3,
    ION_NITRATE_VOLTAGE_P2_M3,
    ION_NITRATE_VOLTAGE_P3_M3};
float ionNitrateVoltage_m4[ION_NO_POINTS] = {
    ION_NITRATE_VOLTAGE_P1_M4,
    ION_NITRATE_VOLTAGE_P2_M4,
    ION_NITRATE_VOLTAGE_P3_M4};

float ionPotassiumVoltage_m1[ION_NO_POINTS] = {
    ION_POTASSIUM_VOLTAGE_P1_M1,
    ION_POTASSIUM_VOLTAGE_P2_M1,
    ION_POTASSIUM_VOLTAGE_P3_M1};
float ionPotassiumVoltage_m2[ION_NO_POINTS] = {
    ION_POTASSIUM_VOLTAGE_P1_M2,
    ION_POTASSIUM_VOLTAGE_P2_M2,
    ION_POTASSIUM_VOLTAGE_P3_M2};
float ionPotassiumVoltage_m3[ION_NO_POINTS] = {
    ION_POTASSIUM_VOLTAGE_P1_M3,
    ION_POTASSIUM_VOLTAGE_P2_M3,
    ION_POTASSIUM_VOLTAGE_P3_M3};
float ionPotassiumVoltage_m4[ION_NO_POINTS] = {
    ION_POTASSIUM_VOLTAGE_P1_M4,
    ION_POTASSIUM_VOLTAGE_P2_M4,
    ION_POTASSIUM_VOLTAGE_P3_M4};

IonCalibrationPoints calciumPoints(ionCalciumVoltage_m1, ionCalciumVoltage_m2, ionCalciumVoltage_m3, ionCalciumVoltage_m4, ionConcentrationPoints);
IonCalibrationPoints nitratePoints(ionNitrateVoltage_m1, ionNitrateVoltage_m2, ionNitrateVoltage_m3, ionNitrateVoltage_m4, ionConcentrationPoints);
IonCalibrationPoints potassiumPoints(ionPotassiumVoltage_m1, ionPotassiumVoltage_m2, ionPotassiumVoltage_m3, ionPotassiumVoltage_m4, ionConcentrationPoints);

GenericIonSensor calciumSensor = GenericIonSensorFactory::build(ION_SOCKET_A, calciumPoints);
GenericIonSensor nitrateSensor = GenericIonSensorFactory::build(ION_SOCKET_D, nitratePoints);
GenericIonSensor potassiumSensor = GenericIonSensorFactory::build(ION_SOCKET_B, potassiumPoints);

IonCalibrationPoints *ionMeasureCalibrationPoints[NO_ION_SENSORS] = {&calciumPoints, &nitratePoints, &potassiumPoints};
GenericIonSensor *ionSensorsBus[NO_ION_SENSORS] = {&calciumSensor, &nitrateSensor, &potassiumSensor};

StaticJsonDocument<1024> jsonDocument;
BatteryInfo Battery;
TemperatureInfo Temperature;
timestamp_t Time;
char timeString[50];
IonMeasures measures;

void setup()
{
#if !PYTHON_GRAPH_OUT_ENABLE
  USB.println(F("Configuring ION..."));
#endif
  configure();
#if !PYTHON_GRAPH_OUT_ENABLE
  USB.println(F("Reading ION..."));
#endif
  awaitTimeBackground(MINUTES_TO_MILLIS(CONCENTRATION_CALCULATION_MINUTES), ionsProcessFunc);
  USB.println(F("Creating json output"));
  buildMeasuresJson();
  USB.println(F("Posting to server data"));
  sendDataToServer();
  PWR.deepSleep("31:00:00:00", RTC_OFFSET, RTC_ALM1_MODE1, ALL_OFF);
}

void loop() {}

void configure()
{
  USB.ON();
#if !PYTHON_GRAPH_OUT_ENABLE
  USB.println(F("   RTC: ON"));
  RTC.ON();
  USB.println(F("   4G: ON"));
  _4G.ON();
  _4G.set_APN(_4G_APN_HOST, _4G_APN_USER, _4G_APN_PASS);
#endif
#if !PYTHON_GRAPH_OUT_ENABLE
  USB.println(F("   SmartWaterBoard: ON"));
#endif
  SWIonsBoard.ON();
  pinMode(DIGITAL8, OUTPUT);
  digitalWrite(DIGITAL8, LOW);
#if !PYTHON_GRAPH_OUT_ENABLE
  USB.println(F("   Get time from 4G"));
  getTimeFrom4G();
  IonUtils.begin();
  calciumPoints.serializeCalibrationToUSB("Calcium");
  nitratePoints.serializeCalibrationToUSB("Nitrate");
  potassiumPoints.serializeCalibrationToUSB("Potassium");
#endif
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

  for (size_t i = 0; i < 50; i++)
    timeString[i] = 0x00;
  sprintf(timeString, "20%i-%02i-%02iT%02i:%02i:%02iZ",
          Time.year, Time.month, Time.day, Time.hour, Time.minute, Time.second);
}

void updateIonsConcentration()
{
  for (uint8_t i = 0; i < NO_ION_SENSORS; i++)
  {
    ionSensorsBus[i]->calculateConcentration();
  }
}

void sendDataToServer()
{
  _4G.httpSetContentType("application/json");
  _4G.http(Wasp4G::HTTP_POST, SERVER_HOST, SERVER_PORT, SERVER_RESOURCE, http_data);
}

void getTimeFrom4G()
{
  bool isConnected = timeoutFunction(MINUTES_TO_MILLIS(1), [](long time) -> bool {
    return _4G.checkConnection(1);
  });
  if (!isConnected)
  {
    while (1)
    {
      USB.println("Can't connect please reset ION device");
      delay(1000);
    }
  }
  _4G.setTimeFrom4G();
}

void awaitTimeBackground(long _delay, void (*process)(long))
{
  long start_time = millis();
  long actual_time = millis();
  while ((actual_time - start_time) < _delay)
  {
    process(_delay - (actual_time - start_time));
    actual_time = millis();
  }
}

bool timeoutFunction(long timeout, bool (*process)(long))
{
  long start_time = millis();
  long actual_time = millis();
  while ((actual_time - start_time) < timeout)
  {
#if !PYTHON_GRAPH_OUT_ENABLE
    if (process(timeout - (actual_time - start_time)))
#else
    if (process((actual_time - start_time)))
#endif
    {
      return true;
    }
    actual_time = millis();
  }
  return false;
}

void ionsProcessFunc(long restingTime)
{
#if !PYTHON_GRAPH_OUT_ENABLE
  USB.println(F(" Update ion concentration measures"));
#endif
  updateIonsConcentration();

  measures.calciumConcentration = calciumSensor.concentration();
  measures.calciumVoltage = calciumSensor.voltage();
  measures.nitrateConcentration = nitrateSensor.concentration();
  measures.nitrateVoltage = nitrateSensor.voltage();
  measures.potassiumConcentration = potassiumSensor.concentration();
  measures.potassiumVoltage = potassiumSensor.voltage();
  measures.batteryLevel = Battery.getChargePercent();
  measures.temperature = Temperature.read();

#if !PYTHON_GRAPH_OUT_ENABLE
  if (restingTime < 0)
  {
    USB.println(F(" Resting seconds: 0s"));
  }
  else
  {
    USB.print(F(" Resting seconds: "));
    USB.print(restingTime / 1000);
    USB.println(F("s"));
  }
#else
  USB.print(restingTime);
  USB.print(" ");
#endif
  measures.serializeToUSB();
#if !PYTHON_GRAPH_OUT_ENABLE
  Battery.printStatus();
  Temperature.printStatus();
  USB.println();
#endif
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

template <typename Tarray>
void printArray(Tarray *array, uint8_t len)
{
  USB.print(F("[ "));
  for (uint8_t i = 0; i < len; i++)
  {
    USB.print(array[i]);
    USB.print(F(" "));
  }
  USB.print(F("]"));
}