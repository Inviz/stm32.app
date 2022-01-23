#ifndef INC_SYSTEM_CANOPEN
#define INC_SYSTEM_CANOPEN

#ifdef __cplusplus
extern "C" {
#endif

#include "core/app.h"
#include "transport/can.h"

#include <CANopen.h>
#include "OD.h"



#define NMT_CONTROL                                                                                                                        \
    CO_NMT_STARTUP_TO_OPERATIONAL                                                                                                          \
    | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION
#define FIRST_HB_TIME 501
#define SDO_SRV_TIMEOUT_TIME 1000
#define SDO_CLI_TIMEOUT_TIME 500
#define SDO_CLI_BLOCK true
#define OD_STATUS_BITS NULL

#define CO_GET_CNT(obj) OD_CNT_##obj
#define OD_GET(entry, index) OD_ENTRY_##entry

#define app_error_report(app, errorBit, errorCode, index) CO_errorReport(app->canopen->instance->em, errorBit, errorCode, index)
#define app_error_reset(app, errorBit, errorCode, index) CO_errorReset(app->canopen->instance->em, errorBit, errorCode, index)

#define device_error_report(device, errorBit, errorCode) CO_errorReport(device->app->canopen->instance->em, errorBit, errorCode, device->index)
#define device_error_reset(device, errorBit, errorCode) CO_errorReset(device->app->canopen->instance->em, errorBit, errorCode, device->index)


/* Start of autogenerated OD types */
/* 0x6020: System CANopen
   CANOpen framework */
typedef struct system_canopen_properties {
    uint8_t parameter_count;
    uint16_t can_index; // Values other than zero will prevent device from initializing 
    uint8_t can_fifo_index; // Values other than zero will prevent device from initializing 
    uint8_t green_led_port;
    uint8_t green_led_pin;
    uint8_t red_led_port;
    uint8_t red_led_pin;
    uint16_t first_hb_time;
    uint16_t sdo_server_timeout; // in MS 
    uint16_t sdo_client_timeout; // in MS 
    uint8_t phase;
    uint8_t node_id;
    uint32_t bitrate;
} system_canopen_properties_t;
/* End of autogenerated OD types */

struct system_canopen {
    device_t *device;
    system_canopen_properties_t *properties;
    transport_can_t *can;
    CO_t *instance;
} ;


extern device_methods_t system_canopen_methods;


/* Start of autogenerated OD accessors */
typedef enum system_canopen_properties_properties {
  SYSTEM_CANOPEN_CAN_INDEX = 0x1,
  SYSTEM_CANOPEN_CAN_FIFO_INDEX = 0x2,
  SYSTEM_CANOPEN_GREEN_LED_PORT = 0x3,
  SYSTEM_CANOPEN_GREEN_LED_PIN = 0x4,
  SYSTEM_CANOPEN_RED_LED_PORT = 0x5,
  SYSTEM_CANOPEN_RED_LED_PIN = 0x6,
  SYSTEM_CANOPEN_FIRST_HB_TIME = 0x7,
  SYSTEM_CANOPEN_SDO_SERVER_TIMEOUT = 0x8,
  SYSTEM_CANOPEN_SDO_CLIENT_TIMEOUT = 0x9,
  SYSTEM_CANOPEN_PHASE = 0x10,
  SYSTEM_CANOPEN_NODE_ID = 0x11,
  SYSTEM_CANOPEN_BITRATE = 0x12
} system_canopen_properties_properties_t;

OD_ACCESSORS(system, canopen, properties, phase, SYSTEM_CANOPEN_PHASE, uint8_t, u8) /* 0x60XX0a: {} */
OD_ACCESSORS(system, canopen, properties, node_id, SYSTEM_CANOPEN_NODE_ID, uint8_t, u8) /* 0x60XX0b: {} */
OD_ACCESSORS(system, canopen, properties, bitrate, SYSTEM_CANOPEN_BITRATE, uint32_t, u32) /* 0x60XX0c: {} */
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif