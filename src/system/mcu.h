#ifndef INC_SYSTEM_MCU
#define INC_SYSTEM_MCU

#ifdef __cplusplus
extern "C" {
#endif

#include "core/app.h"

/* Start of autogenerated OD types */
/* 0x6000: System MCUnull */
typedef struct {
    uint32_t disabled;
    char family[8];
    char board_type[10];
    uint32_t storage_index;
} system_mcu_config_t;
/* End of autogenerated OD types */

struct system_mcu {
    device_t *device;
    system_mcu_config_t *config;
    device_t *storage;
};

extern device_callbacks_t system_mcu_callbacks;

/* Start of autogenerated OD accessors */
#define SUBIDX_MCU_DISABLED 0x1
#define SUBIDX_MCU_FAMILY 0x2
#define SUBIDX_MCU_BOARD_TYPE 0x3
#define SUBIDX_MCU_STORAGE_INDEX 0x4

/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
