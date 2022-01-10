#include "blank.h"

/* Start of autogenerated OD accessors */
/* End of autogenerated OD accessors */

static ODR_t OD_write_device_blank_property(OD_stream_t *stream, const void *buf, OD_size_t count,
                                            OD_size_t *countWritten) {
    device_blank_t *blank = stream->object;
    (void)blank;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static int device_blank_validate(OD_entry_t *config_entry) {
    device_blank_config_t *config = (device_blank_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    (void)config;
    if (false) {
        return CO_ERROR_OD_PARAMETERS;
    }
    return 0;
}

static int device_blank_construct(device_blank_t *blank, device_t *device) {
    blank->config = (device_blank_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);
    return blank->config->disabled;
}

static int device_blank_start(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static int device_blank_stop(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static int device_blank_pause(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static int device_blank_resume(device_blank_t *blank) {
    (void)blank;
    return 0;
}
static int device_blank_link(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static int device_blank_phase(device_blank_t *blank, device_phase_t phase) {
    (void)blank;
    (void)phase;
    return 0;
}

device_callbacks_t device_blank_callbacks = {.validate = device_blank_validate,
                                             .construct = (int (*)(void *, device_t *))device_blank_construct,
                                             .link = (int (*)(void *))device_blank_link,
                                             .start = (int (*)(void *))device_blank_start,
                                             .stop = (int (*)(void *))device_blank_stop,
                                             .pause = (int (*)(void *))device_blank_pause,
                                             .resume = (int (*)(void *))device_blank_resume,
                                             //.accept = (int (*)(void *, device_t *device, void *channel))device_blank_accept,
                                             .phase = (int (*)(void *, device_phase_t phase))device_blank_phase,
                                             .write_values = OD_write_device_blank_property};
