#include "canopen.h"

/* Start of autogenerated OD accessors */
OD_ACCESSORS(system, canopen, values, node_id, SUBIDX_CANOPEN_NODE_ID, uint8_t, u8)   /* 0x80XX01: values */
OD_ACCESSORS(system, canopen, values, bitrate, SUBIDX_CANOPEN_BITRATE, uint16_t, u16) /* 0x80XX02: values */
/* End of autogenerated OD accessors */

static void app_thread_canopen_notify(app_thread_t *thread);
static void system_canopen_initialize_callbacks(system_canopen_t *canopen);

static ODR_t OD_write_system_canopen_property(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    system_canopen_t *canopen = stream->object;
    (void)canopen;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static int system_canopen_validate(OD_entry_t *config_entry) {
    system_canopen_config_t *config = (system_canopen_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    (void)config;
    if (false) {
        return CO_ERROR_OD_PARAMETERS;
    }
    return 0;
}

static int system_canopen_construct(system_canopen_t *canopen, device_t *device) {
    canopen->config = (system_canopen_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);

    /* Allocate memory */
    uint32_t heapMemoryUsed = 0;
    canopen->instance = CO_new(NULL, &heapMemoryUsed);
    if (canopen->instance == NULL) {
        log_printf("Error: Can't allocate memory\n");
        return CO_ERROR_OUT_OF_MEMORY;
    } else {
        if (heapMemoryUsed == 0) {
            log_printf("Config - Static memory\n");
        } else {
            log_printf("Config - On heap (%ubytes)\n", (unsigned int)heapMemoryUsed);
        }
    }

    /*
        log_printf("Config - Storage...\n");

        CO_ReturnError_t err;
        err = CO_storageAbstract_init(&CO_storage, canopen->instance->CANmodule, NULL, OD_ENTRY_H1010_storeParameters,
       OD_ENTRY_H1011_restoreDefaultParameters, storageEntries, storageEntriesCount, storageInitError);

        if (false && err != CO_ERROR_NO && err != CO_ERROR_DATA_CORRUPT) {
            error_printf("Error: Storage %d\n", (int)*storageInitError);
            return err;
        }

        if (storageInitError != 0) {
            initError = storageInitError;
        }
        */
    return 0;
}

static int system_canopen_destruct(system_canopen_t *canopen) {
    CO_CANsetConfigurationMode(canopen->instance);
    CO_delete(canopen->instance);
    return 0;
}

static bool_t system_canopen_store_lss(system_canopen_t *canopen, uint8_t node_id, uint16_t bitrate) {
    log_printf("Config - Store LSS #%i @ %ikbps...\n", node_id, bitrate);
    system_canopen_set_node_id(canopen, node_id);
    system_canopen_set_bitrate(canopen, bitrate);
    return 0;
}

static int system_canopen_start(system_canopen_t *canopen) {
    CO_ReturnError_t err;
    uint32_t errInfo = 0;

    /* Enter CAN configuration. */
    log_printf("Config - Communication...\n");
    canopen->instance->CANmodule->CANnormal = false;
    CO_CANsetConfigurationMode(canopen->instance);
    CO_CANmodule_disable(canopen->instance->CANmodule);

    /* Pass CAN configuration to CANopen driver */
    canopen->instance->CANmodule->port = canopen->can->device->seq == 0 ? CAN1 : CAN2;
    canopen->instance->CANmodule->rxFifoIndex = canopen->config->can_fifo_index;
    canopen->instance->CANmodule->sjw = canopen->can->config->sjw;
    canopen->instance->CANmodule->prop = canopen->can->config->prop;
    canopen->instance->CANmodule->brp = canopen->can->config->brp;
    canopen->instance->CANmodule->ph_seg1 = canopen->can->config->ph_seg1;
    canopen->instance->CANmodule->ph_seg2 = canopen->can->config->ph_seg2;
    canopen->instance->CANmodule->bitrate = canopen->can->config->bitrate;

    /* Initialize CANopen driver */
    err = CO_CANinit(canopen->instance, canopen->instance, canopen->values->bitrate);
    if (false && err != CO_ERROR_NO) {
        error_printf("Error: CAN initialization failed: %d\n", err);
        return err;
    }

    /* Engage LSS configuration */
    CO_LSS_address_t lssAddress = {.identity = {.vendorID = OD_PERSIST_COMM.x1018_identity.vendor_ID,
                                                .productCode = OD_PERSIST_COMM.x1018_identity.productCode,
                                                .revisionNumber = OD_PERSIST_COMM.x1018_identity.revisionNumber,
                                                .serialNumber = OD_PERSIST_COMM.x1018_identity.serialNumber}};

    err = CO_LSSinit(canopen->instance, &lssAddress, &canopen->values->node_id, &canopen->values->bitrate);
    CO_LSSslave_initCfgStoreCallback(canopen->instance->LSSslave, canopen, (bool_t(*)(void *, uint8_t, uint16_t))system_canopen_store_lss);

    if (err != CO_ERROR_NO) {
        error_printf("Error: LSS slave initialization failed: %d\n", err);
        return err;
    }

    system_canopen_initialize_callbacks(canopen);

    /* Initialize CANopen itself */
    err = CO_CANopenInit(canopen->instance,                   /* CANopen object */
                         NULL,                                /* alternate NMT */
                         NULL,                                /* alternate em */
                         OD,                                  /* Object dictionary */
                         OD_STATUS_BITS,                      /* Optional OD_statusBit */
                         NMT_CONTROL,                         /* CO_NMT_control_t */
                         canopen->config->first_hb_time,      /* firstHBTime_ms */
                         canopen->config->sdo_client_timeout, /* SDOserverTimeoutTime_ms */
                         canopen->config->sdo_client_timeout, /* SDOclientTimeoutTime_ms */
                         SDO_CLI_BLOCK,                       /* SDOclientBlockTransfer */
                         canopen->values->node_id, &errInfo);

    if (err == CO_ERROR_OD_PARAMETERS) {
        log_printf("CANopen - Error in Object Dictionary entry 0x%lX\n", errInfo);
    } else {
        log_printf("CANopen - Initialization failed: %d  0x%lX\n", err, errInfo);
    }

    /* Emergency errors */
    if (!canopen->instance->nodeIdUnconfigured) {
        if (errInfo != 0) {
            CO_errorReport(canopen->instance->em, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_DATA_SET, errInfo);
        }
        // if (storageInitError != 0) {
        //    CO_errorReport(canopen->instance->em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE, storageInitError);
        //}
    }

    if (err == CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        return CO_ERROR_NO;
    }

    /* start CAN */
    CO_CANsetNormalMode(canopen->instance->CANmodule);

    err = CO_CANopenInitPDO(canopen->instance, canopen->instance->em, OD, canopen->values->node_id, &errInfo);
    return 0;
}

static int system_canopen_stop(system_canopen_t *canopen) {
    log_printf("Config - Unloading...\n");
    CO_CANsetConfigurationMode((void *)&canopen->instance);
    CO_delete(canopen->instance);
    return 0;
}

static int system_canopen_pause(system_canopen_t *canopen) {
    (void)canopen;
    return 0;
}

static int system_canopen_resume(system_canopen_t *canopen) {
    (void)canopen;
    return 0;
}

static int system_canopen_link(system_canopen_t *canopen) {
    device_link(canopen->device, (void **)&canopen->can, canopen->config->can_index, NULL);
    return 0;
}

static int system_canopen_async_tick(system_canopen_t *canopen, void *argument, device_tick_t *tick, app_thread_t *thread) {
    (void)argument;

    uint32_t us_since_last = (thread->current_time - tick->last_time) * US_PER_TICK;
    uint32_t us_until_next = -1;
    switch (CO_process(canopen->instance, false, us_since_last, &us_until_next)) {
    case CO_RESET_COMM:
        device_set_phase(canopen->device->app->device, DEVICE_RESETTING);
        break;
    case CO_RESET_APP:
    case CO_RESET_QUIT:
        device_set_phase(canopen->device, DEVICE_RESETTING);
        break;
    default:
        break;
    }

    if (us_until_next != (uint32_t)-1) {
        app_thread_tick_schedule(thread, tick, thread->current_time + us_until_next / US_PER_TICK);
    }
    return 0;
}

/* CANopen accepts its input from interrupts */
static int system_canopen_input_tick(system_canopen_t *canopen, void *argument, device_tick_t *tick, app_thread_t *thread) {
    (void) argument;
    uint32_t us_since_last = (thread->current_time - tick->last_time) * 1000;
    uint32_t us_until_next = -1;
    CO_LOCK_OD(canopen->instance->CANmodule);
    if (!canopen->instance->nodeIdUnconfigured && canopen->instance->CANmodule->CANnormal) {
        bool_t syncWas = false;

#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
        syncWas = CO_process_SYNC(canopen->instance, us_since_last, &us_until_next);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
        CO_process_RPDO(canopen->instance, syncWas, us_since_last, &us_until_next);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_TPDO_ENABLE
        CO_process_TPDO(canopen->instance, syncWas, us_since_last, &us_until_next);
#endif
        (void)syncWas;
    }
    CO_UNLOCK_OD(canopen->instance->CANmodule);
    if (us_until_next != (uint32_t)-1) {
        app_thread_tick_schedule(thread, tick, thread->current_time + us_until_next / US_PER_TICK);
    }
    return 0;
}

static int system_canopen_idle_tick(system_canopen_t *canopen, void *argument, device_tick_t *tick, app_thread_t *thread) {
    (void) argument;
    (void) tick;
    (void) thread;
    uint8_t LED_red = CO_LED_RED(canopen->instance->LEDs, CO_LED_CANopen);
    uint8_t LED_green = CO_LED_GREEN(canopen->instance->LEDs, CO_LED_CANopen);

    (void)LED_red;
    (void)LED_green;
    return 0;
}

static int system_canopen_phase(system_canopen_t *canopen, device_phase_t phase) {
    (void)canopen;
    (void)phase;
    return 0;
}

/* Publish a wakeup event to thread for immediate processing */
static void app_thread_canopen_notify(app_thread_t *thread) {
    system_canopen_t *canopen = thread->device->app->canopen;

    app_event_t event = {
        .type = APP_EVENT_MESSAGE_CANOPEN,
        .producer = canopen->device,
        .consumer = canopen->device
    };
    // canopen messages are safe to send in any order and it is desirable to handle them asap
    app_thread_publish(thread, &event);
}

device_callbacks_t system_canopen_callbacks = {
    .validate = system_canopen_validate,
    .construct = (int (*)(void *, device_t *))system_canopen_construct,
    .destruct = (int (*)(void *))system_canopen_destruct,
    .link = (int (*)(void *))system_canopen_link,
    .start = (int (*)(void *))system_canopen_start,
    .stop = (int (*)(void *))system_canopen_stop,
    .pause = (int (*)(void *))system_canopen_pause,
    .resume = (int (*)(void *))system_canopen_resume,

    .input_tick = (int (*)(void *, app_event_t *, device_tick_t *, app_thread_t *))system_canopen_input_tick,
    .async_tick = (int (*)(void *, app_event_t *, device_tick_t *, app_thread_t *))system_canopen_async_tick,
    .idle_tick = (int (*)(void *, app_event_t *, device_tick_t *, app_thread_t *))system_canopen_idle_tick,

    .phase = (int (*)(void *, device_phase_t phase))system_canopen_phase,
    .write_values = OD_write_system_canopen_property};


static void system_canopen_initialize_callbacks(system_canopen_t *canopen) {
    app_t *app = canopen->device->app;

    /* Mainline tasks */
    if (CO_GET_CNT(EM) == 1) {
        CO_EM_initCallbackPre(canopen->instance->em, (void *)&app->threads->input, (void (*)(void *))app_thread_canopen_notify);
    }
    if (CO_GET_CNT(NMT) == 1) {
        CO_NMT_initCallbackPre(canopen->instance->NMT, (void *)&app->threads->input, (void (*)(void *))app_thread_canopen_notify);
    }
#if (CO_CONFIG_SRDO) & CO_CONFIG_SRDO_SRV_ENABLE
    for (int16_t i = 0; i < CO_GET_CNT(SRDO); i++) {
        CO_SRDO_initCallbackPre(&canopen->instance->SRDO[i], (void *)&app->threads->input, (void (*)(void *))app_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_HB_CONS) & CO_CONFIG_HB_CONS_ENABLE
    if (CO_GET_CNT(HB_CONS) == 1) {
        CO_HBconsumer_initCallbackPre(canopen->instance->HBCons, (void *)&app->threads->input, (void (*)(void *))app_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_TIME) & CO_CONFIG_TIME_ENABLE
    if (CO_GET_CNT(TIME) == 1) {
        CO_TIME_initCallbackPre(canopen->instance->TIME, (void *)&app->threads->input, (void (*)(void *))app_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_SDO_CLI) & CO_CONFIG_SDO_CLI_ENABLE
    for (int16_t i = 0; i < CO_GET_CNT(SDO_CLI); i++) {
        CO_SDOclient_initCallbackPre(&canopen->instance->SDOclient[i], (void *)&app->threads->input,
                                     (void (*)(void *))app_thread_canopen_notify);
    }
#endif
    for (int16_t i = 0; i < CO_GET_CNT(SDO_SRV); i++) {
        CO_SDOserver_initCallbackPre(&canopen->instance->SDOserver[i], (void *)&app->threads->input,
                                     (void (*)(void *))app_thread_canopen_notify);
    }
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_MASTER
    CO_LSSmaster_initCallbackPre(canopen->instance->LSSmaster, (void *)&app->threads->input, (void (*)(void *))app_thread_canopen_notify);
#endif
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_SLAVE
    CO_LSSslave_initCallbackPre(canopen->instance->LSSslave, (void *)&app->threads->input, (void (*)(void *))app_thread_canopen_notify);
#endif
/* Processing tasks */
#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
    if (CO_GET_CNT(SYNC) == 1) {
        CO_SYNC_initCallbackPre(canopen->instance->SYNC, (void *)&app->threads->async, (void (*)(void *))app_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
    for (int i = 0; i < CO_NO_RPDO; i++) {
        CO_RPDO_initCallbackPre(c & anopen->instance->RPDO[i], (void *)&app->threads->async, app_thread_canopen_notify);
    }
#endif
}
