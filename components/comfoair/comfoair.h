#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"
#include "messages.h"

namespace esphome {
namespace comfoair {
class ComfoAirComponent : public climate::Climate, public PollingComponent, public uart::UARTDevice {
public:

  // Poll every 1s
  ComfoAirComponent() : 
  Climate(), 
  PollingComponent(6000),
  UARTDevice() { }

  /// Return the traits of this controller.
  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes({
      climate::CLIMATE_MODE_FAN_ONLY
    });
    traits.set_supports_two_point_target_temperature(false);
    traits.set_supports_action(false);
    traits.set_visual_min_temperature(12);
    traits.set_visual_max_temperature(29);
    /// Ensures valid target temperature steps
    traits.set_visual_target_temperature_step(0.5f);
    traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
    }); 
    return traits;
  }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    if (call.get_fan_mode().has_value()) {
      int level;

      this->fan_mode = *call.get_fan_mode();
      switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_HIGH:
          level = 0x04;
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          level = 0x03;
          break;
        case climate::CLIMATE_FAN_LOW:
          level = 0x02;
          break;
        case climate::CLIMATE_FAN_OFF:
          level = 0x01;
          break;
        default:
          level = -1;
          break;
      }

      if (level >= 0) {
        set_level_(level);
      }

    }
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = *call.get_target_temperature();
      set_comfort_temperature_(this->target_temperature);
    }

    this->publish_state();
  }

  void update()  {
    switch(update_counter_) {
      case 0:
        get_fan_status_();
        break;
      case 1:
        get_temperature_();
        break;
      case 2:
        get_bypass_control_status_();
        break;
    }

    update_counter_++;
    if (update_counter_ > 8)
      update_counter_ = 0;
  }

  void loop() override {
    while (this->available() != 0) {
      this->read_byte(&this->data_[this->data_index_]);
      auto check = this->check_byte_();
      if (!check.has_value()) {

        // finished
        if (this->data_[COMFOAIR_MSG_ACK_IDX] != COMFOAIR_MSG_ACK) {
          this->write_byte(COMFOAIR_MSG_PREFIX);
          this->write_byte(COMFOAIR_MSG_ACK);
          this->flush();
          this->parse_data_();
        }
        this->data_index_ = 0;
      } else if (!*check) {
        // wrong data
        ESP_LOGV(TAG, "Byte %i of received data frame is invalid.", this->data_index_);
        this->data_index_ = 0;
      } else {
        // next byte
        this->data_index_++;
      }
    }
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  void reset_filter(void) {
    ESP_LOGI(TAG, "Resetting filter");
    uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {0, 0, 0, 1};
    this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
  }

  void set_name(const char* value) {this->name = value;}
  void set_uart_component(uart::UARTComponent *parent) {this->set_uart_parent(parent);}

protected:

  void set_level_(int level) {
    if (level < 0 || level > 4) {
      ESP_LOGI(TAG, "Ignoring invalid level request: %i", level);
      return;
    }

    ESP_LOGI(TAG, "Setting level to: %i", level);
    {
      uint8_t command[COMFOAIR_SET_LEVEL_LENGTH] = {(uint8_t) level};
      this->write_command_(COMFOAIR_SET_LEVEL_REQUEST, command, sizeof(command));
    }
  }

  void set_comfort_temperature_(float temperature) {
    if (temperature < 12.0f || temperature > 29.0f) {
      ESP_LOGI(TAG, "Ignoring invalid temperature request: %i", temperature);
      return;
    }

    ESP_LOGI(TAG, "Setting temperature to: %4.2f", temperature);
    {
      uint8_t command[COMFOAIR_SET_COMFORT_TEMPERATURE_LENGTH] = {0x00, (uint8_t) ((temperature + 20.0f) * 2.0f)};
      this->write_command_(COMFOAIR_SET_COMFORT_TEMPERATURE_REQUEST, command, sizeof(command));
    }
  }

  void write_command_(const uint8_t command, const uint8_t *command_data, uint8_t command_data_length) {
    this->write_byte(COMFOAIR_MSG_PREFIX);
    this->write_byte(COMFOAIR_MSG_HEAD);
    this->write_byte(0x00);
    this->write_byte(command);
    this->write_byte(command_data_length);
    if (command_data_length > 0) {
      this->write_array(command_data, command_data_length);
      this->write_byte((command + command_data_length + comfoair_checksum_(command_data, command_data_length)) & 0xff);
    } else {
      this->write_byte(comfoair_checksum_(&command, 1));
    }
    this->write_byte(COMFOAIR_MSG_PREFIX);
    this->write_byte(COMFOAIR_MSG_TAIL);
    this->flush();
}

  uint8_t comfoair_checksum_(const uint8_t *command_data, uint8_t length) const {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
      sum += command_data[i];
    }
    return sum + 0xad;
  }

  optional<bool> check_byte_() const {
    uint8_t index = this->data_index_;
    uint8_t byte = this->data_[index];

    if (index == 0) {
      return byte == COMFOAIR_MSG_PREFIX;
    }

    if (index == 1) {
      if (byte == COMFOAIR_MSG_ACK) {
        return {};
      } else {
        return byte == COMFOAIR_MSG_HEAD;
      }
    }

    if (index == 2) {
      return byte == 0x00;
    }

    if (index < COMFOAIR_MSG_HEAD_LENGTH) {
      return true;
    }

    uint8_t data_length = this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX];

    if ((COMFOAIR_MSG_HEAD_LENGTH + data_length + COMFOAIR_MSG_TAIL_LENGTH) > sizeof(this->data_)) {
      ESP_LOGW(TAG, "ComfoAir message too large");
      return false;
    }

    if (index < COMFOAIR_MSG_HEAD_LENGTH + data_length) {
      return true;
    }

    if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length) {
      // checksum is without checksum bytes
      uint8_t checksum = comfoair_checksum_(this->data_ + 2, COMFOAIR_MSG_HEAD_LENGTH + data_length - 2);
      if (checksum != byte) {
        ESP_LOGW(TAG, "ComfoAir Checksum doesn't match: 0x%02X!=0x%02X", byte, checksum);
        return false;
      }
      return true;
    }

    if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length + 1) {
      return byte == COMFOAIR_MSG_PREFIX;
    }

    if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length + 2) {
      if (byte != COMFOAIR_MSG_TAIL) {
        return false;
      }
    }

    return {};
  }

  void parse_data_() {
    this->status_clear_warning();
    uint8_t *msg = &this->data_[COMFOAIR_MSG_HEAD_LENGTH];

    switch (this->data_[COMFOAIR_MSG_IDENTIFIER_IDX]) {
      case COMFOAIR_GET_FAN_STATUS_RESPONSE: {
        if (this->fan_supply_air_percentage != nullptr) {
          this->fan_supply_air_percentage->publish_state(msg[0]);
        }
        if (this->fan_exhaust_air_percentage != nullptr) {
          this->fan_exhaust_air_percentage->publish_state(msg[1]);
        }
        if (this->fan_speed_supply != nullptr) {
          this->fan_speed_supply->publish_state((float) msg[2] * 20.0f);
        }
        if (this->fan_speed_exhaust != nullptr) {
          this->fan_speed_exhaust->publish_state((float) msg[3] * 20.0f);
        }

        // Fan Speed
        switch(msg[8]) {
          case 0x00:
            this->fan_mode = climate::CLIMATE_FAN_LOW;
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
          case 0x01:
            this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
          case 0x02:
            this->fan_mode = climate::CLIMATE_FAN_HIGH;
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
        }
        break;
      }
      case COMFOAIR_GET_TEMPERATURE_RESPONSE: {
        if (this->is_bypass_valve_open != nullptr) {
          this->is_bypass_valve_open->publish_state(msg[0] != 0);
        }

        // T1 / outside air
        if (this->outside_air_temperature != nullptr) {
          this->outside_air_temperature->publish_state((float) msg[3] / 2.0f - 20.0f);
        }

        // T2 / supply air
        if (this->supply_air_temperature != nullptr) {
          this->supply_air_temperature->publish_state(0);
        }

        // T3 / return air
        if (this->return_air_temperature != nullptr) {
          this->return_air_temperature->publish_state((float) msg[4] / 2.0f - 20.0f);
          this->current_temperature = (float) msg[4] / 2.0f - 20.0f;
        }

        // T4 / exhaust air
        if (this->exhaust_air_temperature != nullptr) {
          this->exhaust_air_temperature->publish_state((float) msg[5] / 2.0f - 20.0f);
        }
        break;
      }
      case COMFOAIR_GET_BYPASS_CONTROL_RESPONSE: {
        // comfort temperature
        this->target_temperature = (float) msg[1] / 2.0f - 20.0f;
        this->publish_state();
        break;
      }
    }
  }

  void get_fan_status_() {
    if (this->fan_supply_air_percentage != nullptr ||
        this->fan_exhaust_air_percentage != nullptr ||
        this->fan_speed_supply != nullptr ||
        this->fan_speed_exhaust != nullptr) {
      ESP_LOGD(TAG, "getting fan status");
      this->write_command_(COMFOAIR_GET_FAN_STATUS_REQUEST, nullptr, 0);
    }
  }

  void get_bypass_control_status_() {
    ESP_LOGD(TAG, "getting bypass control");
    this->write_command_(COMFOAIR_GET_BYPASS_CONTROL_REQUEST, nullptr, 0);
  }

  void get_temperature_() {
    if (this->outside_air_temperature != nullptr ||
      this->supply_air_temperature != nullptr ||
      this->return_air_temperature != nullptr ||
      this->outside_air_temperature != nullptr) {
      ESP_LOGD(TAG, "getting temperature");
      this->write_command_(COMFOAIR_GET_TEMPERATURE_REQUEST, nullptr, 0);
    }
  }

  uint8_t get_uint8_t_(uint8_t start_index) const {
    return this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index];
  }

  uint16_t get_uint16_(uint8_t start_index) const {
    return (uint16_t(this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index + 1] | this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index] << 8));
  }

  uint8_t data_[30];
  uint8_t data_index_{0};
  int8_t update_counter_{-1};

  const char* name{0};

public: 
  sensor::Sensor *fan_supply_air_percentage{nullptr};
  sensor::Sensor *fan_exhaust_air_percentage{nullptr};
  sensor::Sensor *fan_speed_supply{nullptr};
  sensor::Sensor *fan_speed_exhaust{nullptr};
  sensor::Sensor *outside_air_temperature{nullptr};
  sensor::Sensor *supply_air_temperature{nullptr};
  sensor::Sensor *return_air_temperature{nullptr};
  sensor::Sensor *exhaust_air_temperature{nullptr};
  binary_sensor::BinarySensor *is_bypass_valve_open{nullptr};

  void set_fan_supply_air_percentage(sensor::Sensor *fan_supply_air_percentage) {this->fan_supply_air_percentage = fan_supply_air_percentage;};
  void set_fan_exhaust_air_percentage(sensor::Sensor *fan_exhaust_air_percentage) {this->fan_exhaust_air_percentage =fan_exhaust_air_percentage; };
  void set_fan_speed_supply(sensor::Sensor *fan_speed_supply) {this->fan_speed_supply =fan_speed_supply; };
  void set_fan_speed_exhaust(sensor::Sensor *fan_speed_exhaust) {this->fan_speed_exhaust =fan_speed_exhaust; };
  void set_is_bypass_valve_open(binary_sensor::BinarySensor *is_bypass_valve_open) {this->is_bypass_valve_open =is_bypass_valve_open; };
  void set_outside_air_temperature(sensor::Sensor *outside_air_temperature) {this->outside_air_temperature =outside_air_temperature; };
  void set_supply_air_temperature(sensor::Sensor *supply_air_temperature) {this->supply_air_temperature =supply_air_temperature; };
  void set_return_air_temperature(sensor::Sensor *return_air_temperature) {this->return_air_temperature =return_air_temperature; };
  void set_exhaust_air_temperature(sensor::Sensor *exhaust_air_temperature) {this->exhaust_air_temperature =exhaust_air_temperature; };
};

}  // namespace comfoair
}  // namespace esphome
