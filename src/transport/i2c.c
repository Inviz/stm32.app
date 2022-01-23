#include "i2c.h"
#include "lib/dma.h"

/* Start of autogenerated OD accessors */

/* End of autogenerated OD accessors */

static ODR_t OD_write_transport_i2c_property(OD_stream_t *stream, const void *buf, OD_size_t count,
                                            OD_size_t *countWritten) {
    transport_i2c_t *i2c = stream->object;
    (void)i2c;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t i2c_validate(OD_entry_t *config_entry) {
    transport_i2c_config_t *config = (transport_i2c_config_t *)OD_getPtr(config_entry, 0x00, 0, NULL);
    (void)config;
    if (false) {
        return CO_ERROR_OD_PARAMETERS;
    }
    return 0;
}

static void i2c_tx_dma_start(transport_i2c_t *i2c, uint8_t *data, uint16_t size) {
    i2c_tx_dma_stop(i2c);
    dma_periphery_tx_start((uint32_t) & (I2C_DR(i2c->address)), i2c->config->dma_tx_unit, i2c->config->dma_tx_stream, i2c->config->dma_tx_channel, data, size);
    i2c_enable_txdma(i2c->address);
}

static app_signal_t i2c_phase_constructing(transport_i2c_t *i2c, device_t *device) {
    i2c->config = (transport_i2c_config_t *)OD_getPtr(device->config, 0x00, 0, NULL);
    return i2c->config->disabled;
}

static app_signal_t i2c_phase_starting(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_phase_stoping(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_phase_pausing(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_phase_resuming(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_tick(transport_i2c_t *i2c, uint32_t time_passed, uint32_t *next_tick) {
    (void)i2c;
    (void)time_passed;
    (void)next_tick;
    return 0;
}

static app_signal_t i2c_phase_linking(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_phase(transport_i2c_t *i2c, device_phase_t phase) {
    (void)i2c;
    (void)phase;
    return 0;
}

device_methods_t transport_i2c_methods = {.validate = i2c_validate,
                                             .phase_constructing = (app_signal_t (*)(void *, device_t *))i2c_phase_constructing,
                                             .phase_linking = (app_method_t) i2c_phase_linking,
                                             .phase_starting = (app_method_t) i2c_phase_starting,
                                             .phase_stoping = (app_method_t) i2c_phase_stoping,
                                             .phase_pausing = (app_method_t) i2c_phase_pausing,
                                             .phase_resuming = (app_method_t) i2c_phase_resuming,
                                             //.accept = (int (*)(void *, device_t *device, void *channel))transport_i2c_accept,
                                             .callback_phase = (app_signal_t (*)(void *, device_phase_t phase))i2c_phase,
                                             .write_values = OD_write_transport_i2c_property};
