#ifndef INC_CORE_APP
#define INC_CORE_APP
/* Generic types for all apps.

Any app type can be cast to this type and get access to generic properties properties and objects */

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "lib/debug.h"
#include "CANopen.h"
#include "core/types.h"
#include "core/device.h"
#include "core/thread.h"
#include "enums.h"

#define malloc(size) pvPortMalloc(size)
#define free(pointer) vPortFree(pointer)

/* Start of autogenerated OD types */
/* 0x3000: Core App
   Configuration of global object */
typedef struct app_properties {
    uint8_t parameter_count;
    uint32_t timer_index; // Index of a timer used for generic medium-precision scheduling (1us) 
    uint32_t storage_index;
    uint32_t mcu_index; // Main MCU device index 
    uint32_t canopen_index; // Main CANopen device 
    uint8_t phase;
} app_properties_t;
/* End of autogenerated OD types */


struct app {
    device_t *device;
    app_properties_t *properties;
    size_t device_count;
    OD_t *dictionary;
    app_threads_t *threads;
    system_mcu_t *mcu;
    system_canopen_t *canopen;
    module_timer_t *timer;
    app_thread_t *current_thread;
};

enum app_signal {
    APP_SIGNAL_OK,
    APP_SIGNAL_FAILURE,

    APP_SIGNAL_TIMEOUT,
    APP_SIGNAL_TIMER,

    APP_SIGNAL_DMA_ERROR,
    APP_SIGNAL_DMA_TRANSFERRING,
    APP_SIGNAL_DMA_IDLE,

    APP_SIGNAL_RX_COMPLETE,
    APP_SIGNAL_TX_COMPLETE,

    APP_SIGNAL_CATCHUP,
    APP_SIGNAL_RESCHEDULE,
    APP_SIGNAL_INCOMING,
    APP_SIGNAL_BUSY,
    APP_SIGNAL_NOT_FOUND,
    APP_SIGNAL_UNCONFIGURED,

    APP_SIGNAL_OUT_OF_MEMORY
};


// Initialize array of all devices found in OD that can be initialized
int app_allocate(app_t **app, OD_t *od, size_t (*enumerator)(app_t *app, OD_t *od, device_t *devices));
// Destruct all devices and release memory
int app_free(app_t **app);
// Transition all devices to given state
void app_set_phase(app_t *app, device_phase_t phase);

size_t app_device_type_enumerate(app_t *app, OD_t *od, device_type_t type, device_methods_t *methods, size_t struct_size,
                                  device_t *destination, size_t offset);

/* Find device by index in the global list of registered devices */
device_t *app_device_find(app_t *app, uint16_t index);
/* Find device by type in the global list of registered devices */
device_t *app_device_find_by_type(app_t *app, uint16_t type);
/* Return device from a global array by its index */
device_t *app_device_find_by_number(app_t *app, uint8_t number);
/* Get numeric index of a device in a global array */
uint8_t app_device_find_number(app_t *app, device_t *device);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif