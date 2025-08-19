#include "balboa_spa.h"
#include "esphome/core/ring_buffer.h"

namespace esphome
{
  namespace balboa_spa
  {

    // Define the logger tag
    static const char *TAG = "balboa_spa";
    // float get_setup_priority() const override { return esphome::setup_priority::LATE; }

    // char have_config = 0; //stages: 0-> want it; 1-> requested it; 2-> got it; 3-> further processed it
    // char have_faultlog = 0; //stages: 0-> want it; 1-> requested it; 2-> got it; 3-> further processed it
    // char have_filtersettings = 0; //stages: 0-> want it; 1-> requested it; 2-> got it; 3-> further processed it
    // char faultlog_minutes = 0; //temp logic so we only get the fault log once per 5 minutes
    // char filtersettings_minutes = 0; //temp logic so we only get the filter settings once per 5 minutes

    // struct {
    //   uint8_t jet1 :2;
    //   uint8_t jet2 :2;
    //   uint8_t blower :1;
    //   uint8_t light :1;
    //   uint8_t restmode:1;
    //   uint8_t highrange:1;
    //   uint8_t heater:1;
    //   uint8_t hour :5;
    //   uint8_t minutes :6;
    //   uint8_t circ :1;
    // } SpaState;

    // struct {
    //   uint8_t pump1 :2; //this could be 1=1 speed; 2=2 speeds
    //   uint8_t pump2 :2;
    //   uint8_t pump3 :2;
    //   uint8_t pump4 :2;
    //   uint8_t pump5 :2;
    //   uint8_t pump6 :2;
    //   uint8_t light1 :1;
    //   uint8_t light2 :1;
    //   uint8_t circ :1;
    //   uint8_t blower :1;
    //   uint8_t mister :1;
    //   uint8_t aux1 :1;
    //   uint8_t aux2 :1;
    //   uint8_t temp_scale :1; //0 -> Farenheit, 1-> Celcius
    // } SpaConfig;

    // struct {
    //   uint8_t totEntry :5;
    //   uint8_t currEntry :5;
    //   uint8_t faultCode :6;
    //   String faultMessage;
    //   uint8_t daysAgo :8;
    //   uint8_t hour :5;
    //   uint8_t minutes :6;
    // } SpaFaultLog;

    // struct {
    //   uint8_t filt1Hour :5;
    //   uint8_t filt1Minute :6;
    //   uint8_t filt1DurationHour :5;
    //   uint8_t filt1DurationMinute :6;
    //   uint8_t filt2Enable :1;
    //   uint8_t filt2Hour :5;
    //   uint8_t filt2Minute :6;
    //   uint8_t filt2DurationHour :5;
    //   uint8_t filt2DurationMinute :6;

    // } SpaFilterSettings;

    void BalboaSpa::set_temp_sensor(sensor::Sensor *temp_sensor)
    {
      this->temp_sensor_ = temp_sensor;
    }
    void BalboaSpa::set_heater_binary_sensor(binary_sensor::BinarySensor *heater_binary_sensor)
    {
      this->heater_binary_sensor_ = heater_binary_sensor;
    }
    void BalboaSpa::set_circ_binary_sensor(binary_sensor::BinarySensor *circ_binary_sensor)
    {
      this->circ_binary_sensor_ = circ_binary_sensor;
    }
    void BalboaSpa::set_blower_binary_sensor(binary_sensor::BinarySensor *blower_binary_sensor)
    {
      this->blower_binary_sensor_ = blower_binary_sensor;
    }
    void BalboaSpa::set_rest_binary_sensor(binary_sensor::BinarySensor *rest_binary_sensor)
    {
      this->rest_binary_sensor_ = rest_binary_sensor;
    }
    void BalboaSpa::set_high_range_binary_sensor(binary_sensor::BinarySensor *high_range_binary_sensor)
    {
      this->high_range_binary_sensor_ = high_range_binary_sensor;
    }
    void BalboaSpa::set_fault_text_sensor(text_sensor::TextSensor *fault_text_sensor)
    {
      this->fault_sensor_ = fault_text_sensor;
    }
    // void BalboaSpa::set_jet1_binary_sensor(binary_sensor::BinarySensor *jet1_binary_sensor)
    // {
    //   this->jet1_binary_sensor_ = jet1_binary_sensor;
    // }
    // void BalboaSpa::set_jet2_binary_sensor(binary_sensor::BinarySensor *jet2_binary_sensor)
    // {
    //   this->jet2_binary_sensor_ = jet2_binary_sensor;
    // }
    // void BalboaSpa::set_light_binary_sensor(binary_sensor::BinarySensor *light_binary_sensor)
    // {
    //   this->light_binary_sensor_ = light_binary_sensor;
    // }

    void BalboaSpa::register_sensor_callback(uint8_t datapoint, const std::function<void(float)> &func)
    {
      switch (datapoint)
      {
      case 40:
        this->target_temp_state_update_ = func;
        break;
      case 41:
        this->current_temp_state_update_ = func;
        break;
      }
    }
        void BalboaSpa::register_binary_sensor_callback(uint8_t datapoint, const std::function<void(bool)> &func)
        {
          switch (datapoint)
          {
          case 20:
            this->light_state_update_ = func;
            break;
          case 21:
            this->heater_state_update_ = func;
            break;
          case 22:
            this->circulation_pump_state_update_ = func;
            break;
          case 23:
            this->rest_state_update_ = func;
            break;
          case 24:
            this->jet1_state_update_ = func;
            break;
          case 25:
            this->jet2_state_update_ = func;
            break;
          case 26:
            this->blower_state_update_ = func;
            break;
          }
        }

        uint8_t BalboaSpa::crc8(const std::vector<uint8_t> &data)
        {
          unsigned long crc;
          int i, bit;
          uint8_t length = data.size();

          crc = 0x02;
          for (i = 0; i < length; i++)
          {
            crc ^= data[i];
            for (bit = 0; bit < 8; bit++)
            {
              if ((crc & 0x80) != 0)
              {
                crc <<= 1;
                crc ^= 0x7;
              }
              else
              {
                crc <<= 1;
              }
            }
          }

          return crc ^ 0x02;
        }

        void BalboaSpa::ID_request()
        {
          this->Q_out.push_back(0xFE);
          this->Q_out.push_back(0xBF);
          this->Q_out.push_back(0x01);
          this->Q_out.push_back(0x02);
          this->Q_out.push_back(0xF1);
          this->Q_out.push_back(0x73);

          rs485_send();
        }

        void BalboaSpa::ID_ack()
        {
          this->Q_out.push_back(id);
          this->Q_out.push_back(0xBF);
          this->Q_out.push_back(0x03);

          rs485_send();
        }

        void BalboaSpa::rs485_send()
        {
          // Add telegram length
          this->Q_out.insert(this->Q_out.begin(), this->Q_out.size() + 2);

          // Add CRC
          this->Q_out.push_back(crc8(this->Q_out));

          // Wrap telegram in SOF/EOF
          this->Q_out.insert(this->Q_out.begin(), 0x7E);
          this->Q_out.push_back(0x7E);

          for (i = 0; i < this->Q_out.size(); i++)
          {
            write(this->Q_out[i]);
          }

          // print_msg(this->Q_out);

          flush();

          // DEBUG: print_msg(this->Q_out);
          this->Q_out.clear();
        }

        // void BalboaSpa::print_msg(std::vector<uint8_t> &data) {
        //   std::string s;
        //   //for (i = 0; i < (this->Q_in[1] + 2); i++) {
        //   for (i = 0; i < data.size(); i++) {
        //     x = this->Q_in[i];
        //     // if (x < 0x0A) s += "0";
        //     // s += String(x, HEX);
        //     // s += " ";
        //   }
        //   yield();
        // }

        void BalboaSpa::decodeSettings()
        {
          ESP_LOGD("Spa/config/status", "Got config");
          SpaConfig.pump1 = this->Q_in[5] & 0x03;
          this->SpaConfig.pump2 = (this->Q_in[5] & 0x0C) >> 2;
          SpaConfig.pump3 = (this->Q_in[5] & 0x30) >> 4;
          SpaConfig.pump4 = (this->Q_in[5] & 0xC0) >> 6;
          SpaConfig.pump5 = (this->Q_in[6] & 0x03);
          SpaConfig.pump6 = (this->Q_in[6] & 0xC0) >> 6;
          SpaConfig.light1 = (this->Q_in[7] & 0x03);
          SpaConfig.light2 = (this->Q_in[7] >> 2) & 0x03;
          SpaConfig.circ = ((this->Q_in[8] & 0x80) != 0);
          SpaConfig.blower = ((this->Q_in[8] & 0x03) != 0);
          SpaConfig.mister = ((this->Q_in[9] & 0x30) != 0);
          SpaConfig.aux1 = ((this->Q_in[9] & 0x01) != 0);
          SpaConfig.aux2 = ((this->Q_in[9] & 0x02) != 0);
          SpaConfig.temp_scale = this->Q_in[3] & 0x01; // Read temperature scale - 0 -> Farenheit, 1-> Celcius
          // ESP_LOGD("Spa/config/pumps1", String(SpaConfig.pump1).c_str());
          // ESP_LOGD("Spa/config/pumps2", String(SpaConfig.pump2).c_str());
          // ESP_LOGD("Spa/config/pumps3", String(SpaConfig.pump3).c_str());
          // ESP_LOGD("Spa/config/pumps4", String(SpaConfig.pump4).c_str());
          // ESP_LOGD("Spa/config/pumps5", String(SpaConfig.pump5).c_str());
          // ESP_LOGD("Spa/config/pumps6", String(SpaConfig.pump6).c_str());
          // ESP_LOGD("Spa/config/light1", String(SpaConfig.light1).c_str());
          // ESP_LOGD("Spa/config/light2", String(SpaConfig.light2).c_str());
          // ESP_LOGD("Spa/config/circ", String(SpaConfig.circ).c_str());
          // ESP_LOGD("Spa/config/blower", String(SpaConfig.blower).c_str());
          // ESP_LOGD("Spa/config/mister", String(SpaConfig.mister).c_str());
          // ESP_LOGD("Spa/config/aux1", String(SpaConfig.aux1).c_str());
          // ESP_LOGD("Spa/config/aux2", String(SpaConfig.aux2).c_str());
          // ESP_LOGD("Spa/config/temp_scale", String(SpaConfig.temp_scale).c_str());
          have_config = 2;
        }

        void BalboaSpa::decodeState()
        {
          // std::string s;
          float e = 0.0;
          float d = 0.0;
          float c = 0.0;

          // 25:Flag Byte 20 - Set Temperature
          e = this->Q_in[25];
          d = e / 2;

          // ESP_LOGD("Spa/target_temp/state", String(d, 2).c_str());
          // if (target_temp_sensor_ != nullptr)
          // this.target_temp = d;

          if (this->target_temp_state_update_ != nullptr)
          {
            this->target_temp_state_update_(d);
          }
          // 7:Flag Byte 2 - Actual temperature
          if (this->Q_in[7] != 0xFF)
          {
            e = this->Q_in[7];
            d = e / 2;
            if (c > 0)
            {
              if ((d > c * 1.2) || (d < c * 0.8))
                d = c; // remove spurious readings greater or less than 20% away from previous read
            }

            // ESP_LOGD("Spa/temperature/state", String(d, 2).c_str());
            if (this->current_temp_state_update_ != nullptr)
            {
              this->current_temp_state_update_(d);
            }
            c = d;
          }
          else
          {
            d = 0;
          }
          // REMARK Move upper publish to HERE to get 0 for unknown temperature

          // 8:Flag Byte 3 Hour & 9:Flag Byte 4 Minute => Time
          // if (this->Q_in[8] < 10) s = "0"; else s = "";
          SpaState.hour = this->Q_in[8];
          // s += String(this->Q_in[8]) + ":";
          // if (this->Q_in[9] < 10) s += "0";
          // s += String(this->Q_in[9]);
          SpaState.minutes = this->Q_in[9];
          // ESP_LOGD("Spa/time/state", s.c_str());
          sethour = SpaState.hour;
          setminute = SpaState.minutes;
          if (hour_sensor_ != nullptr)
            hour_sensor_->publish_state(SpaState.hour);
          if (minute_sensor_ != nullptr)
            minute_sensor_->publish_state(SpaState.minutes);
          if (time_text_sensor_ != nullptr)
          {
            char buf[6];
            // Ensure 0-padded 24h HH:MM
            snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)SpaState.hour, (unsigned)SpaState.minutes);
            time_text_sensor_->publish_state(std::string(buf));
          }

          // 10:Flag Byte 5 - Heating Mode
          switch (this->Q_in[10])
          {
          case 0:
            ESP_LOGD("Spa/heatingmode/state", STRON); // Ready
            SpaState.restmode = 0;
            if (this->rest_state_update_ != nullptr)
              this->rest_state_update_(0);
            break;
          case 3: // Ready-in-Rest
            SpaState.restmode = 0;
            if (this->rest_state_update_ != nullptr)
              this->rest_state_update_(false);
            break;
          case 1:
            ESP_LOGD("Spa/heatingmode/state", STROFF); // Rest
            SpaState.restmode = 1;
            if (this->rest_state_update_ != nullptr)
              this->rest_state_update_(true);
            break;
          }

          // 15:Flags Byte 10 / Heat status, Temp Range
          d = bitRead(this->Q_in[15], 4);
          if (d == 0)
          {
            ESP_LOGD("Spa/heatstate/state", STROFF);
            if (this->heater_state_update_ != nullptr)
              this->heater_state_update_(false);
            SpaState.heater = 0;
          }
          else if (d == 1 || d == 2)
          {
            ESP_LOGD("Spa/heatstate/state", STRON);
            if (this->heater_state_update_ != nullptr)
              this->heater_state_update_(true);
            SpaState.heater = 1;
          }
          d = bitRead(this->Q_in[15], 2);
          if (d == 0)
          {
            ESP_LOGD("Spa/highrange/state", STROFF); // LOW
            if (high_range_binary_sensor_ != nullptr)
              high_range_binary_sensor_->publish_state(0);
            SpaState.highrange = 0;
          }
          else if (d == 1)
          {
            ESP_LOGD("Spa/highrange/state", STRON); // HIGH
            if (high_range_binary_sensor_ != nullptr)
              high_range_binary_sensor_->publish_state(1);
            SpaState.highrange = 1;
          }

          // 16:Flags Byte 11
          if (bitRead(this->Q_in[16], 1) == 1)
          {
            ESP_LOGD("Spa/jet_1/state", STRON);
            if (this->jet1_state_update_ != nullptr)
              this->jet1_state_update_(true);
            SpaState.jet1 = 1;
          }
          else
          {
            ESP_LOGD("Spa/jet_1/state", STROFF);
            if (this->jet1_state_update_ != nullptr)
              this->jet1_state_update_(false);
            SpaState.jet1 = 0;
          }

          if (bitRead(this->Q_in[16], 3) == 1)
          {
            ESP_LOGD("Spa/jet_2/state", STRON);
            if (this->jet2_state_update_ != nullptr)
              this->jet2_state_update_(true);
            SpaState.jet2 = 1;
          }
          else
          {
            ESP_LOGD("Spa/jet_2/state", STROFF);
            if (this->jet2_state_update_ != nullptr)
              this->jet2_state_update_(false);
            SpaState.jet2 = 0;
          }

          // 18:Flags Byte 13
          if (bitRead(this->Q_in[18], 1) == 1)
          {
            ESP_LOGD("Spa/circ/state", STRON);
            SpaState.circ = 1;
            if (this->circulation_pump_state_update_ != nullptr)
            {
              this->circulation_pump_state_update_(true);
            }
          }
          else
          {
            ESP_LOGD("Spa/circ/state", STROFF);
            SpaState.circ = 0;
            if (this->circulation_pump_state_update_ != nullptr)
            {
              this->circulation_pump_state_update_(false);
            }
          }

          if (bitRead(this->Q_in[18], 2) == 1)
          {
            ESP_LOGD("Spa/blower/state", STRON);
            SpaState.blower = 1;
            if (this->blower_state_update_ != nullptr)
            {
              this->blower_state_update_(true);
            }
          }
          else
          {
            ESP_LOGD("Spa/blower/state", STROFF);
            SpaState.blower = 0;
            if (this->blower_state_update_ != nullptr)
            {
              this->blower_state_update_(false);
            }
          }
          // 19:Flags Byte 14
          if (this->Q_in[19] == 0x03)
          {
            ESP_LOGD("Spa/light/state", STRON);
            SpaState.light = 1;
            if (this->light_state_update_ != nullptr)
              this->light_state_update_(true);
          }
          else
          {
            ESP_LOGD("Spa/light/state", STROFF);
            SpaState.light = 0;
            if (this->light_state_update_ != nullptr)
              this->light_state_update_(false);
          }

          last_state_crc = this->Q_in[this->Q_in[1]];
        }

        void BalboaSpa::decodeFilterSettings()
        {
          // std::string s;
          // std::string d;
          // std::string payld;

          SpaFilterSettings.filt1Hour = this->Q_in[5];
          SpaFilterSettings.filt1Minute = this->Q_in[6];
          SpaFilterSettings.filt1DurationHour = this->Q_in[7];
          SpaFilterSettings.filt1DurationMinute = this->Q_in[8];
          SpaFilterSettings.filt2Enable = bitRead(this->Q_in[9], 7);                          // check
          SpaFilterSettings.filt2Hour = this->Q_in[9] ^ (SpaFilterSettings.filt2Enable << 7); // check
          SpaFilterSettings.filt2Minute = this->Q_in[10];
          SpaFilterSettings.filt2DurationHour = this->Q_in[11];
          SpaFilterSettings.filt2DurationMinute = this->Q_in[12];

          // //Filter 1 time conversion
          // if (SpaFilterSettings.filt1Hour < 10) s = "0"; else s = "";
          // s = String(SpaFilterSettings.filt1Hour) + ":";
          // if (SpaFilterSettings.filt1Minute < 10) s += "0";
          // s += String(SpaFilterSettings.filt1Minute);

          // if (SpaFilterSettings.filt1DurationHour < 10) d = "0"; else d = "";
          // d = String(SpaFilterSettings.filt1DurationHour) + ":";
          // if (SpaFilterSettings.filt1DurationMinute < 10) d += "0";
          // d += String(SpaFilterSettings.filt1DurationMinute);

          // payld = "{\"start\":\""+s+"\",\"duration\":\""+d+"\"}";
          // ESP_LOGD("Spa/filter1/state", payld.c_str());

          // //Filter 2 time conversion
          // if (SpaFilterSettings.filt2Hour < 10) s = "0"; else s = "";
          // s += String(SpaFilterSettings.filt2Hour) + ":";
          // if (SpaFilterSettings.filt2Minute < 10) s += "0";
          // s += String(SpaFilterSettings.filt2Minute);

          // if (SpaFilterSettings.filt2DurationHour < 10) d = "0"; else d = "";
          // d += String(SpaFilterSettings.filt2DurationHour) + ":";
          // if (SpaFilterSettings.filt2DurationMinute < 10) d += "0";
          // d += String(SpaFilterSettings.filt2DurationMinute);
          // if ((int)(SpaFilterSettings.filt2Enable) == 1) ESP_LOGD("Spa/filter2_enabled/state", STRON); else ESP_LOGD("Spa/filter2_enabled/state", STROFF);

          // payld = "{\"start\":\""+s+"\",\"duration\":\""+d+"\"}";
          // ESP_LOGD("Spa/filter2/state", payld.c_str());

          have_filtersettings = 2;
        }

        void BalboaSpa::decodeFault()
        {
          SpaFaultLog.totEntry = this->Q_in[5];
          SpaFaultLog.currEntry = this->Q_in[6];
          SpaFaultLog.faultCode = this->Q_in[7];
          switch (SpaFaultLog.faultCode)
          { // this is a inelegant way to do it, a lookup table would be better
          case 15:
            SpaFaultLog.faultMessage = "Sensors are out of sync";
            break;
          case 16:
            SpaFaultLog.faultMessage = "The water flow is low";
            break;
          case 17:
            SpaFaultLog.faultMessage = "The water flow has failed";
            break;
          case 18:
            SpaFaultLog.faultMessage = "The settings have been reset";
            break;
          case 19:
            SpaFaultLog.faultMessage = "Priming Mode";
            break;
          case 20:
            SpaFaultLog.faultMessage = "The clock has failed";
            break;
          case 21:
            SpaFaultLog.faultMessage = "The settings have been reset";
            break;
          case 22:
            SpaFaultLog.faultMessage = "Program memory failure";
            break;
          case 26:
            SpaFaultLog.faultMessage = "Sensors are out of sync -- Call for service";
            break;
          case 27:
            SpaFaultLog.faultMessage = "The heater is dry";
            break;
          case 28:
            SpaFaultLog.faultMessage = "The heater may be dry";
            break;
          case 29:
            SpaFaultLog.faultMessage = "The water is too hot";
            break;
          case 30:
            SpaFaultLog.faultMessage = "The heater is too hot";
            break;
          case 31:
            SpaFaultLog.faultMessage = "Sensor A Fault";
            break;
          case 32:
            SpaFaultLog.faultMessage = "Sensor B Fault";
            break;
          case 34:
            SpaFaultLog.faultMessage = "A pump may be stuck on";
            break;
          case 35:
            SpaFaultLog.faultMessage = "Hot fault";
            break;
          case 36:
            SpaFaultLog.faultMessage = "The GFCI test failed";
            break;
          case 37:
            SpaFaultLog.faultMessage = "Standby Mode (Hold Mode)";
            break;
          default:
            SpaFaultLog.faultMessage = "Unknown error";
            break;
          }
          this->fault_sensor_->publish_state(SpaFaultLog.faultMessage);
          SpaFaultLog.daysAgo = this->Q_in[8];
          SpaFaultLog.hour = this->Q_in[9];
          SpaFaultLog.minutes = this->Q_in[10];
          // ESP_LOGD("Spa/fault/Entries", String(SpaFaultLog.totEntry).c_str());
          // ESP_LOGD("Spa/fault/Entry", String(SpaFaultLog.currEntry).c_str());
          // ESP_LOGD("Spa/fault/Code", String(SpaFaultLog.faultCode).c_str());
          // ESP_LOGD("Spa/fault/Message", SpaFaultLog.faultMessage.c_str());
          // ESP_LOGD("Spa/fault/DaysAgo", String(SpaFaultLog.daysAgo).c_str());
          // ESP_LOGD("Spa/fault/Hours", String(SpaFaultLog.hour).c_str());
          // ESP_LOGD("Spa/fault/Minutes", String(SpaFaultLog.minutes).c_str());
          have_faultlog = 2;
          // ESP_LOGD("Spa/debug/have_faultlog", "have the faultlog, #2");
        }
        void BalboaSpa::on_set_temp(float temp)
        {
          if (temp >= 26 || temp <= 40)
          {
            settemp = temp * 2;
            send = 0xff;
          }
        }

        void BalboaSpa::on_set_time(int hour, int minute)
        {
          if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59)
          {
            sethour = static_cast<uint8_t>(hour);
            setminute = static_cast<uint8_t>(minute);
            send = 0x21; // mark for time update
          }
        }

        // void on_set_hour(int hour) {
        //   if(hour >= 0 || hour <= 23) {
        //     sethour = hour;
        //     send = 0x21;
        //   }
        // }

        // void on_set_minute(int minute) {
        //   if(minute >= 0 || minute <= 59) {
        //     setminute = minute;
        //     send = 0x21;
        //   }
        // }
        // void on_toggle_heatingmode() {
        //   send = 0x51;
        // }
        // void on_toggle_range() {
        //   send = 0x50;
        // }
        void BalboaSpa::on_toggle_light()
        {
          send = 0x11;
        }
        void BalboaSpa::on_toggle_jet1()
        {
          send = 0x04;
        }
        void BalboaSpa::on_toggle_jet2()
        {
          send = 0x05;
        }

        void BalboaSpa::setup()
        {
          this->Q_in.clear();
          this->Q_out.clear();
        }

        void BalboaSpa::loop()
        {
          yield();
          while (available())
          {
            x = read();
            this->Q_in.push_back(x);

            // Drop until SOF is seen
            if (this->Q_in.front() != 0x7E)
            {
              this->Q_in.clear();
            }

            // Double SOF-marker, drop last one
            if (this->Q_in[1] == 0x7E && this->Q_in.size() > 1)
              this->Q_in.erase(this->Q_in.begin());

            // Complete package
            // if (x == 0x7E && this->Q_in[0] == 0x7E && this->Q_in[1] != 0x7E) {
            if (x == 0x7E && this->Q_in.size() > 2)
            {

              // Unregistered or yet in progress
              if (id == 0)
              {
                ESP_LOGD("Spa/node/id", "Unregistered");
                // if (this->Q_in[2] == 0xFE) print_msg(this->Q_in);
                // print_msg(this->Q_in);
                // FE BF 02:got new client ID
                if (this->Q_in[2] == 0xFE && this->Q_in[4] == 0x02)
                {
                  id = this->Q_in[5];
                  if (id > 0x2F)
                    id = 0x2F;
                  ESP_LOGD("Spa/node/id", "Got ID, acknowledging");
                  ID_ack();
                  ESP_LOGD("Spa/node/id", "%d", id);
                }

                // FE BF 00:Any new clients?
                if (this->Q_in[2] == 0xFE && this->Q_in[4] == 0x00)
                {
                  ESP_LOGD("Spa/node/id", "Requesting ID");
                  ID_request();
                }
              }
              else if (this->Q_in[2] == id && this->Q_in[4] == 0x06)
              { // we have an ID, do clever stuff
                // id BF 06:Ready to Send
                if (send == 0x21)
                {
                  this->Q_out.push_back(id);
                  this->Q_out.push_back(0xBF);
                  this->Q_out.push_back(0x21);
                  this->Q_out.push_back(sethour);
                  this->Q_out.push_back(setminute);
                }
                else if (send == 0xff)
                {
                  // 0xff marks dirty temperature for now
                  this->Q_out.push_back(id);
                  this->Q_out.push_back(0xBF);
                  this->Q_out.push_back(0x20);
                  this->Q_out.push_back(settemp);
                }
                else if (send == 0x00)
                {
                  if (have_config == 0)
                  { // Get configuration of the hot tub
                    this->Q_out.push_back(id);
                    this->Q_out.push_back(0xBF);
                    this->Q_out.push_back(0x22);
                    this->Q_out.push_back(0x00);
                    this->Q_out.push_back(0x00);
                    this->Q_out.push_back(0x01);
                    ESP_LOGD("Spa/config/status", "Getting config");
                    have_config = 1;
                  }
                  else if (have_faultlog == 0)
                  { // Get the fault log
                    this->Q_out.push_back(id);
                    this->Q_out.push_back(0xBF);
                    this->Q_out.push_back(0x22);
                    this->Q_out.push_back(0x20);
                    this->Q_out.push_back(0xFF);
                    this->Q_out.push_back(0x00);
                    have_faultlog = 1;
                    ESP_LOGD("Spa/debug/have_faultlog", "requesting fault log, #1");
                  }
                  else if ((have_filtersettings == 0) && (have_faultlog == 2))
                  { // Get the filter cycles log once we have the faultlog
                    this->Q_out.push_back(id);
                    this->Q_out.push_back(0xBF);
                    this->Q_out.push_back(0x22);
                    this->Q_out.push_back(0x01);
                    this->Q_out.push_back(0x00);
                    this->Q_out.push_back(0x00);
                    ESP_LOGD("Spa/debug/have_filtersettings", "requesting filter settings, #1");
                    have_filtersettings = 1;
                  }
                  else
                  {
                    // A Nothing to Send message is sent by a client immediately after a Clear to Send message if the client has no messages to send.
                    this->Q_out.push_back(id);
                    this->Q_out.push_back(0xBF);
                    this->Q_out.push_back(0x07);
                  }
                }
                else
                {
                  // Send toggle commands
                  this->Q_out.push_back(id);
                  this->Q_out.push_back(0xBF);
                  this->Q_out.push_back(0x11);
                  this->Q_out.push_back(send);
                  this->Q_out.push_back(0x00);
                }

                rs485_send();
                send = 0x00;
              }
              else if (this->Q_in[2] == id && this->Q_in[4] == 0x2E)
              {
                if (last_state_crc != this->Q_in[this->Q_in[1]])
                {
                  decodeSettings();
                }
              }
              else if (this->Q_in[2] == id && this->Q_in[4] == 0x28)
              {
                if (last_state_crc != this->Q_in[this->Q_in[1]])
                {
                  decodeFault();
                }
              }
              else if (this->Q_in[2] == 0xFF && this->Q_in[4] == 0x13)
              { // FF AF 13:Status Update - Packet index offset 5
                if (last_state_crc != this->Q_in[this->Q_in[1]])
                {
                  decodeState();
                }
              }
              else if (this->Q_in[2] == id && this->Q_in[4] == 0x23)
              { // FF AF 23:Filter Cycle Message - Packet index offset 5
                if (last_state_crc != this->Q_in[this->Q_in[1]])
                {
                  ESP_LOGD("Spa/debug/have_faultlog", "decoding filter settings");
                  decodeFilterSettings();
                }
              }
              else
              {
                // DEBUG for finding meaning
                // if (this->Q_in[2] & 0xFE || this->Q_in[2] == id)
                // print_msg(this->Q_in);
              }

              // Clean up queue
              yield();
              this->Q_in.clear();
            }
            lastrx = millis();
          }
        }

      } // namespace spa_reader
    } // namespace esphome
