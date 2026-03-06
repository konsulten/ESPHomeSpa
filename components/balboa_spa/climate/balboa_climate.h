#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/balboa_spa/balboa_spa.h"

namespace esphome
{
  namespace balboa_spa
  {
    class BalboaClimate : public Component,public climate::Climate
    {
    public:
      void setup() override;
      // void loop() override;
      void control(const climate::ClimateCall &call) override;
      climate::ClimateTraits traits() override
      {
        auto traits = climate::ClimateTraits();
        traits.add_feature_flags(
          esphome::climate::CLIMATE_SUPPORTS_ACTION |
          esphome::climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
        traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
        traits.set_visual_min_temperature(26.0);
        traits.set_visual_max_temperature(40.0);
        traits.set_visual_temperature_step(0.5);
        return traits;
      };

      void set_balboa_parent(BalboaSpa *parent)
      {
        this->parent_ = parent;
      }
    protected:
      BalboaSpa *parent_{nullptr};
     };
  }
}
