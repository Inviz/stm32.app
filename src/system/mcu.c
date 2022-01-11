#include "mcu.h"

/* Start of autogenerated OD accessors */

/* End of autogenerated OD accessors */

static ODR_t OD_write_system_mcu_property(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    system_mcu_t *mcu = stream->object;
    (void)mcu;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static int system_mcu_validate(OD_entry_t *config_entry) {
    system_mcu_config_t *config = (system_mcu_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    (void)config;
    if (false) {
        return CO_ERROR_OD_PARAMETERS;
    }
    return 0;
}

static int system_mcu_phase_constructing(system_mcu_t *mcu, device_t *device) {
    mcu->config = (system_mcu_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);
    return mcu->config->disabled;
}

static int system_mcu_phase_starting(system_mcu_t *mcu) {
    (void) mcu;
#if defined(STM32F1)
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSE8_72MHZ]);
#elif defined(STM32F4)
    rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
#endif
    return 0;
}

static int system_mcu_phase_stoping(system_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static int system_mcu_phase_pausing(system_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static int system_mcu_phase_resuming(system_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static int system_mcu_phase_linking(system_mcu_t *mcu) {
    (void)mcu;
    return device_phase_linking(mcu->device, (void **)&mcu->storage, mcu->config->storage_index, NULL);
}

static int system_mcu_phase(system_mcu_t *mcu, device_phase_t phase) {
    (void)mcu;
    (void)phase;
    return 0;
}

device_methods_t system_mcu_methods = {.validate = system_mcu_validate,
                                           .phase_constructing = (app_signal_t (*)(void *, device_t *))system_mcu_phase_constructing,
                                           .phase_linking = (app_signal_t (*)(void *))system_mcu_phase_linking,
                                           .phase_starting = (app_signal_t (*)(void *))system_mcu_phase_starting,
                                           .phase_stoping = (app_signal_t (*)(void *))system_mcu_phase_stoping,
                                           .phase_pausing = (app_signal_t (*)(void *))system_mcu_phase_pausing,
                                           .phase_resuming = (app_signal_t (*)(void *))system_mcu_phase_resuming,
                                           //.accept = (int (*)(void *, device_t *device, void *channel))system_mcu_accept,
                                           .callback_phase = (app_signal_t (*)(void *, device_phase_t phase))system_mcu_phase,
                                           .write_values = OD_write_system_mcu_property};
