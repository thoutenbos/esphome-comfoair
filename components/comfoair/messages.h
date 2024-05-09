#ifndef __messages_h__
#define __messages_h__

#include <stdint.h>

namespace esphome {
namespace comfoair {

static const char *TAG = "comfoair";

static const uint8_t COMFOAIR_MSG_HEAD_LENGTH = 5;
static const uint8_t COMFOAIR_MSG_TAIL_LENGTH = 3;
static const uint8_t COMFOAIR_MSG_PREFIX = 0x07;
static const uint8_t COMFOAIR_MSG_HEAD = 0xf0;
static const uint8_t COMFOAIR_MSG_TAIL = 0x0f;
static const uint8_t COMFOAIR_MSG_ACK = 0xf3;
static const uint8_t COMFOAIR_MSG_IDENTIFIER_IDX = 3;
static const uint8_t COMFOAIR_MSG_DATA_LENGTH_IDX = 4;
static const uint8_t COMFOAIR_MSG_ACK_IDX = 1;

static const uint8_t COMFOAIR_GET_FAN_STATUS_REQUEST = 0x87;
static const uint8_t COMFOAIR_GET_FAN_STATUS_RESPONSE = 0x86;
static const uint8_t COMFOAIR_GET_FAN_STATUS_LENGTH = 0x06;
static const uint8_t COMFOAIR_GET_VALVE_STATUS_REQUEST = 0x0d;
static const uint8_t COMFOAIR_GET_VALVE_STATUS_RESPONSE = 0x0e;
static const uint8_t COMFOAIR_GET_VALVE_STATUS_LENGTH = 0x04;
static const uint8_t COMFOAIR_GET_TEMPERATURE_REQUEST = 0x85;
static const uint8_t COMFOAIR_GET_TEMPERATURE_RESPONSE = 0x84;
static const uint8_t COMFOAIR_GET_TEMPERATURE_LENGTH = 0x04;
static const uint8_t COMFOAIR_GET_BYPASS_CONTROL_REQUEST = 0x8B;
static const uint8_t COMFOAIR_GET_BYPASS_CONTROL_RESPONSE = 0x8A;
static const uint8_t COMFOAIR_GET_BYPASS_CONTROL_LENGTH = 0x10;

// requests with ACK response
static const uint8_t COMFOAIR_SET_LEVEL_REQUEST = 0xA0;
static const uint8_t COMFOAIR_SET_LEVEL_LENGTH = 0x02;
static const uint8_t COMFOAIR_SET_VENTILATION_LEVEL_REQUEST = 0xcf;
static const uint8_t COMFOAIR_SET_VENTILATION_LEVEL_LENGTH = 0x09;
static const uint8_t COMFOAIR_SET_COMFORT_TEMPERATURE_REQUEST = 0x8D;
static const uint8_t COMFOAIR_SET_COMFORT_TEMPERATURE_LENGTH = 0x0A;
static const uint8_t COMFOAIR_SET_RESET_REQUEST = 0xdb;
static const uint8_t COMFOAIR_SET_RESET_LENGTH = 0x04;

// Specials setters
static const uint8_t COMFOAIR_SET_TEST_MODE_START_REQUEST = 0x01;
static const uint8_t COMFOAIR_SET_TEST_MODE_END_REQUEST = 0x19;
static const uint8_t COMFOAIR_SET_OUTPUTS_REQUEST = 0x05;
static const uint8_t COMFOAIR_SET_ANALOG_OUTPUTS_REQUEST = 0x07;
static const uint8_t COMFOAIR_SET_VALVES_REQUEST = 0x09;

} //namespace comfoair
} //namespace esphome

#endif //__messages_h__
