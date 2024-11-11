#include "esphome.h"
#include <CircularBuffer.h>

#define STRON "ON"
#define STROFF "OFF"

class SpaReader : public Component, public UARTDevice, public CustomAPIDevice {
public:
  SpaReader(UARTComponent *parent) : UARTDevice(parent) {}

  float get_setup_priority() const override { return esphome::setup_priority::LATE; }

  Sensor *temp_sensor = new Sensor();
  Sensor *target_temp_sensor = new Sensor();
  Sensor *jet1_sensor = new Sensor();
  Sensor *jet2_sensor = new Sensor();
  Sensor *blower_sensor = new Sensor();
  Sensor *light_sensor = new Sensor();
  Sensor *restmode_sensor = new Sensor();
  Sensor *highrange_sensor = new Sensor();
  Sensor *hour_sensor = new Sensor();
  Sensor *minute_sensor = new Sensor();
  Sensor *heater_sensor = new Sensor();
  Sensor *circ_sensor = new Sensor();

  CircularBuffer<uint8_t, 35> Q_in;
  CircularBuffer<uint8_t, 35> Q_out;
  uint8_t x, i, j;
  uint8_t last_state_crc = 0x00;
  uint8_t send = 0x00;
  uint8_t settemp = 0x00;
  uint8_t sethour = 0x00;
  uint8_t setminute = 0x00;
  uint8_t id = 0x00;
  unsigned long lastrx = 0;

  char have_config = 0;
  char have_faultlog = 0;
  char have_filtersettings = 0;
  char faultlog_minutes = 0;
  char filtersettings_minutes = 0;

  struct {
    uint8_t jet1 :2;
    uint8_t jet2 :2;
    uint8_t blower :1;
    uint8_t light :1;
    uint8_t restmode:1;
    uint8_t highrange:1;
    uint8_t heater:1;
    uint8_t hour :5;
    uint8_t minutes :6;
    uint8_t circ :1;
  } SpaState;

  struct {
    uint8_t pump1 :2;
    uint8_t pump2 :2;
    uint8_t pump3 :2;
    uint8_t pump4 :2;
    uint8_t pump5 :2;
    uint8_t pump6 :2;
    uint8_t light1 :1;
    uint8_t light2 :1;
    uint8_t circ :1;
    uint8_t blower :1;
    uint8_t mister :1;
    uint8_t aux1 :1;
    uint8_t aux2 :1;
    uint8_t temp_scale :1;
  } SpaConfig;

  struct {
    uint8_t totEntry :5;
    uint8_t currEntry :5;
    uint8_t faultCode :6;
    String faultMessage;
    uint8_t daysAgo :8;
    uint8_t hour :5;
    uint8_t minutes :6;
  } SpaFaultLog;

  struct {
    uint8_t filt1Hour :5;
    uint8_t filt1Minute :6;
    uint8_t filt1DurationHour :5;
    uint8_t filt1DurationMinute :6;
    uint8_t filt2Enable :1;
    uint8_t filt2Hour :5;
    uint8_t filt2Minute :6;
    uint8_t filt2DurationHour :5;
    uint8_t filt2DurationMinute :6;
  } SpaFilterSettings;

  uint8_t crc8(CircularBuffer<uint8_t, 35> &data);
  void ID_request();
  void ID_ack();
  void rs485_send();
  void print_msg(CircularBuffer<uint8_t, 35> &data);
  void decodeSettings();
  void decodeState();
  void decodeFilterSettings();
  void decodeFault();

  void on_set_temp(float temp);
  void on_set_hour(int hour);
  void on_set_minute(int minute);
  void on_toggle_heatingmode();
  void on_toggle_range();
  void on_toggle_light();
  void on_toggle_jet1();
  void on_toggle_jet2();

  void setup() override;
  void loop() override;
};

