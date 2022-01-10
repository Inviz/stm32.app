
#include "epaper.h"
#include "transport/spi.h"

/* Start of autogenerated OD accessors */
OD_ACCESSORS(screen, epaper, values, image_buffer, SUBIDX_EPAPER_IMAGE_BUFFER, int16_t, i16) /* 0x90XX01: values */
OD_ACCESSORS(screen, epaper, values, render_count, SUBIDX_EPAPER_RENDER_COUNT, int16_t, i16) /* 0x90XX02: values */
/* End of autogenerated OD accessors */

/* Epaper needs DC, CS, BUSY, RESET pins set, as well as screen size */
static int screen_epaper_validate(OD_entry_t *config_entry) {
    screen_epaper_config_t *config = (screen_epaper_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    return config->dc_pin == 0 || config->dc_port == 0 || config->cs_port == 0 || config->cs_pin == 0 ||
           config->busy_pin == 0 || config->busy_port == 0 || config->reset_port == 0 || config->reset_pin == 0 ||
           config->width == 0 || config->height == 0;
}

static int screen_epaper_construct(screen_epaper_t *epaper, device_t *device) {
    epaper->config = (screen_epaper_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);
    epaper->values = (screen_epaper_values_t *)OD_getPtr(device->values, 0x01, 0, NULL);
    return 1;//epaper->config->disabled;
}

static int screen_epaper_link(screen_epaper_t *epaper) {
    return device_link(epaper->device, (void **)&epaper->spi, epaper->config->spi_index, NULL);
}

const unsigned char screen_epaper_lut_full_update[] = {
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, // LUT0: BB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00, // LUT1: BW:     VS 0 ~7
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, // LUT2: WB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00, // LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT4: VCOM:   VS 0 ~7

    0x03, 0x03, 0x00, 0x00, 0x02, // TP0 A~D RP0
    0x09, 0x09, 0x00, 0x00, 0x02, // TP1 A~D RP1
    0x03, 0x03, 0x00, 0x00, 0x02, // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00, // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00, // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00, // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00, // TP6 A~D RP6

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

const unsigned char screen_epaper_lut_partial_update[] = {
    // 20 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT0: BB:     VS 0 ~7
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT1: BW:     VS 0 ~7
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT2: WB:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT4: VCOM:   VS 0 ~7

    0x0A, 0x00, 0x00, 0x00, 0x00, // TP0 A~D RP0
    0x00, 0x00, 0x00, 0x00, 0x00, // TP1 A~D RP1
    0x00, 0x00, 0x00, 0x00, 0x00, // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00, // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00, // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00, // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00, // TP6 A~D RP6

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

/* send command */
static void screen_epaper_send_command(screen_epaper_t *epaper, uint8_t cmd) {
    device_gpio_clear(epaper->config->dc_port, epaper->config->dc_pin);
    device_gpio_clear(epaper->config->cs_port, epaper->config->cs_pin);
    transport_spi_send(epaper->spi, cmd);
    device_gpio_set(epaper->config->cs_port, epaper->config->cs_pin);
}

/* Write data */
static void screen_epaper_send_data(screen_epaper_t *epaper, uint8_t data) {
    device_gpio_set(epaper->config->dc_port, epaper->config->dc_pin);
    device_gpio_clear(epaper->config->cs_port, epaper->config->cs_pin);
    transport_spi_send(epaper->spi, data);
    device_gpio_set(epaper->config->cs_port, epaper->config->cs_pin);
}

/* Wait until the busy_pin goes LOW */
static int screen_epaper_set_busy_phase(screen_epaper_t *epaper) {
    if (device_gpio_get(epaper->config->busy_port, epaper->config->busy_pin) == 1) {
        device_set_temporary_phase(epaper->device, DEVICE_BUSY, 10000); // 50000
        return 1;
    } else {
        device_set_phase(epaper->device, DEVICE_RUNNING);
    }
    return 0;
}

/* Reset the device screen */
static int screen_epaper_set_resetting_phase(screen_epaper_t *epaper) {
    epaper->resetting_phase++;

    switch (epaper->resetting_phase) {
        case 1:
            device_gpio_set(epaper->config->reset_port, epaper->config->reset_pin);
            device_set_temporary_phase(epaper->device, DEVICE_RESETTING, 200000);
            break;
        case 2:
            device_gpio_clear(epaper->config->reset_port, epaper->config->reset_pin);
            device_set_temporary_phase(epaper->device, DEVICE_RESETTING, 2000);
            break;
        case 3:
            device_gpio_set(epaper->config->reset_port, epaper->config->reset_pin);
            device_set_temporary_phase(epaper->device, DEVICE_RESETTING, 200000);
            break;
        case 4:
            epaper->resetting_phase = 0;
            device_set_phase(epaper->device, DEVICE_RUNNING);
    }
    return 0;
}

/* Turn On Display */
static void screen_epaper_turn_on(screen_epaper_t *epaper) {
    screen_epaper_send_command(epaper, 0x22);
    screen_epaper_send_data(epaper, 0xC7);
    screen_epaper_send_command(epaper, 0x20);
    screen_epaper_set_busy_phase(epaper);
}

/* Turn On Display in partial mode */
static void screen_epaper_turn_on_part(screen_epaper_t *epaper) {
    screen_epaper_send_command(epaper, 0x22);
    screen_epaper_send_data(epaper, 0x0C);
    screen_epaper_send_command(epaper, 0x20);
    screen_epaper_set_busy_phase(epaper);
}

/* Initialize the e-Paper register */
static void screen_epaper_init_mode(screen_epaper_t *epaper, uint8_t mode) {
    uint8_t count;
    (void) mode;
    //screen_epaper_reset(epaper);

    //if (Mode == screen_epaper_FULL) {
        screen_epaper_set_busy_phase(epaper);
        screen_epaper_send_command(epaper, 0x12); // soft reset
        screen_epaper_set_busy_phase(epaper);

        screen_epaper_send_command(epaper, 0x74); // set analog block control
        screen_epaper_send_data(epaper, 0x54);
        screen_epaper_send_command(epaper, 0x7E); // set digital block control
        screen_epaper_send_data(epaper, 0x3B);

        screen_epaper_send_command(epaper, 0x01); // Driver output control
        screen_epaper_send_data(epaper, 0xF9);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x00);

        screen_epaper_send_command(epaper, 0x11); // data entry mode
        screen_epaper_send_data(epaper, 0x01);

        screen_epaper_send_command(epaper, 0x44); // set Ram-X address start/end position
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x0C); // 0x0C-->(15+1)*8=128

        screen_epaper_send_command(epaper, 0x45); // set Ram-Y address start/end position
        screen_epaper_send_data(epaper, 0xF9);    // 0xF9-->(249+1)=250
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x00);

        screen_epaper_send_command(epaper, 0x3C); // BorderWavefrom
        screen_epaper_send_data(epaper, 0x03);

        screen_epaper_send_command(epaper, 0x2C); // VCOM Voltage
        screen_epaper_send_data(epaper, 0x55);    //

        screen_epaper_send_command(epaper, 0x03);
        screen_epaper_send_data(epaper, screen_epaper_lut_full_update[70]);

        screen_epaper_send_command(epaper, 0x04); //
        screen_epaper_send_data(epaper, screen_epaper_lut_full_update[71]);
        screen_epaper_send_data(epaper, screen_epaper_lut_full_update[72]);
        screen_epaper_send_data(epaper, screen_epaper_lut_full_update[73]);

        screen_epaper_send_command(epaper, 0x3A); // Dummy Line
        screen_epaper_send_data(epaper, screen_epaper_lut_full_update[74]);
        screen_epaper_send_command(epaper, 0x3B); // Gate time
        screen_epaper_send_data(epaper, screen_epaper_lut_full_update[75]);

        screen_epaper_send_command(epaper, 0x32);
        for (count = 0; count < 70; count++) {
            screen_epaper_send_data(epaper, screen_epaper_lut_full_update[count]);
        }

        screen_epaper_send_command(epaper, 0x4E); // set RAM x address count to 0;
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_command(epaper, 0x4F); // set RAM y address count to 0X127;
        screen_epaper_send_data(epaper, 0xF9);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_set_busy_phase(epaper);
    /*} else if (Mode == screen_epaper_PART) {
        screen_epaper_send_command(epaper, 0x2C); // VCOM Voltage
        screen_epaper_send_data(epaper, 0x26);

        screen_epaper_set_busy_phase(epaper);

        screen_epaper_send_command(epaper, 0x32);
        for (count = 0; count < 70; count++) {
            screen_epaper_send_data(epaper, screen_epaper_lut_partial_update[count]);
        }

        screen_epaper_send_command(epaper, 0x37);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x40);
        screen_epaper_send_data(epaper, 0x00);
        screen_epaper_send_data(epaper, 0x00);

        screen_epaper_send_command(epaper, 0x22);
        screen_epaper_send_data(epaper, 0xC0);

        screen_epaper_send_command(epaper, 0x20);
        screen_epaper_set_busy_phase(epaper);

        screen_epaper_send_command(epaper, 0x3C); // BorderWavefrom
        screen_epaper_send_data(epaper, 0x01);
    } else {
        log_printf("error, the Mode is epaper_2IN13_FULL or epaper_2IN13_PART");
    }*/
}

/* Clear screen */
static void screen_epaper_clear(screen_epaper_t *epaper) {
    uint16_t Width, Height;
    Width = (epaper->config->width % 8 == 0) ? (epaper->config->width / 8) : (epaper->config->width / 8 + 1);
    Height = epaper->config->height;

    screen_epaper_send_command(epaper, 0x24);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            screen_epaper_send_data(epaper, 0XFF);
        }
    }

    screen_epaper_turn_on(epaper);
}

/* Sends the image buffer in RAM to e-Paper and displays */
static void screen_epaper_display(screen_epaper_t *epaper, uint8_t *Image) {
    uint16_t Width, Height;
    Width = (epaper->config->width % 8 == 0) ? (epaper->config->width / 8) : (epaper->config->width / 8 + 1);
    Height = epaper->config->height;

    screen_epaper_send_command(epaper, 0x24);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            screen_epaper_send_data(epaper, Image[i + j * Width]);
        }
    }
    screen_epaper_turn_on(epaper);
}

/* The image of the previous frame must be uploaded, otherwise the first few seconds will display an exception. */
static void screen_epaper_display_part_base_image(screen_epaper_t *epaper, uint8_t *Image) {
    uint16_t Width, Height;
    Width = (epaper->config->width % 8 == 0) ? (epaper->config->width / 8) : (epaper->config->width / 8 + 1);
    Height = epaper->config->height;

    uint32_t Addr = 0;
    screen_epaper_send_command(epaper, 0x24);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            Addr = i + j * Width;
            screen_epaper_send_data(epaper, Image[Addr]);
        }
    }
    screen_epaper_send_command(epaper, 0x26);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            Addr = i + j * Width;
            screen_epaper_send_data(epaper, Image[Addr]);
        }
    }
    screen_epaper_turn_on(epaper);
}

static void screen_epaper_display_part(screen_epaper_t *epaper, uint8_t *Image) {
    uint16_t Width, Height;
    Width = (epaper->config->width % 8 == 0) ? (epaper->config->width / 8) : (epaper->config->width / 8 + 1);
    Height = epaper->config->height;
    screen_epaper_send_command(epaper, 0x24);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            screen_epaper_send_data(epaper, Image[i + j * Width]);
        }
    }

    screen_epaper_turn_on_part(epaper);
}

/* Enter sleep mode */
static void screen_epaper_sleep(screen_epaper_t *epaper) {
    screen_epaper_send_command(epaper, 0x22); // POWER OFF
    screen_epaper_send_data(epaper, 0xC3);
    screen_epaper_send_command(epaper, 0x20);

    screen_epaper_send_command(epaper, 0x10); // enter deep sleep
    screen_epaper_send_data(epaper, 0x01);
    //  vDelay(100);
}

static int screen_epaper_destruct(screen_epaper_t *epaper) {
    (void)epaper;
    return 0;
}

/* Reset the device screen */
static int screen_epaper_set_initializing_phase(screen_epaper_t *epaper) {
    epaper->initializing_phase++;

    switch (epaper->initializing_phase) {
        case 1:
            screen_epaper_set_resetting_phase(epaper);
            break;
        case 2:
            screen_epaper_set_busy_phase(epaper);
            break;
        case 3:
            screen_epaper_send_command(epaper, 0x12); // soft reset
            screen_epaper_set_busy_phase(epaper);
            break;
        case 4:
            screen_epaper_send_command(epaper, 0x74); // set analog block control
            screen_epaper_send_data(epaper, 0x54);
            screen_epaper_send_command(epaper, 0x7E); // set digital block control
            screen_epaper_send_data(epaper, 0x3B);

            screen_epaper_send_command(epaper, 0x01); // Driver output control
            screen_epaper_send_data(epaper, 0xF9);
            screen_epaper_send_data(epaper, 0x00);
            screen_epaper_send_data(epaper, 0x00);

            screen_epaper_send_command(epaper, 0x11); // data entry mode
            screen_epaper_send_data(epaper, 0x01);

            screen_epaper_send_command(epaper, 0x44); // set Ram-X address start/end position
            screen_epaper_send_data(epaper, 0x00);
            screen_epaper_send_data(epaper, 0x0C); // 0x0C-->(15+1)*8=128

            screen_epaper_send_command(epaper, 0x45); // set Ram-Y address start/end position
            screen_epaper_send_data(epaper, 0xF9);    // 0xF9-->(249+1)=250
            screen_epaper_send_data(epaper, 0x00);
            screen_epaper_send_data(epaper, 0x00);
            screen_epaper_send_data(epaper, 0x00);

            screen_epaper_send_command(epaper, 0x3C); // BorderWavefrom
            screen_epaper_send_data(epaper, 0x03);

            screen_epaper_send_command(epaper, 0x2C); // VCOM Voltage
            screen_epaper_send_data(epaper, 0x55);    //

            screen_epaper_send_command(epaper, 0x03);
            screen_epaper_send_data(epaper, screen_epaper_lut_full_update[70]);

            screen_epaper_send_command(epaper, 0x04); //
            screen_epaper_send_data(epaper, screen_epaper_lut_full_update[71]);
            screen_epaper_send_data(epaper, screen_epaper_lut_full_update[72]);
            screen_epaper_send_data(epaper, screen_epaper_lut_full_update[73]);

            screen_epaper_send_command(epaper, 0x3A); // Dummy Line
            screen_epaper_send_data(epaper, screen_epaper_lut_full_update[74]);
            screen_epaper_send_command(epaper, 0x3B); // Gate time
            screen_epaper_send_data(epaper, screen_epaper_lut_full_update[75]);

            screen_epaper_send_command(epaper, 0x32);
            for (size_t count = 0; count < 70; count++) {
                screen_epaper_send_data(epaper, screen_epaper_lut_full_update[count]);
            }

            screen_epaper_send_command(epaper, 0x4E); // set RAM x address count to 0;
            screen_epaper_send_data(epaper, 0x00);
            screen_epaper_send_command(epaper, 0x4F); // set RAM y address count to 0X127;
            screen_epaper_send_data(epaper, 0xF9);
            screen_epaper_send_data(epaper, 0x00);
            screen_epaper_set_busy_phase(epaper);
            break;
        case 5:
            epaper->initializing_phase = 0;
            screen_epaper_clear(epaper);
    }
    return 0;
}


static int screen_epaper_start(screen_epaper_t *epaper) {
    device_gpio_configure_input("Busy", epaper->config->busy_port, epaper->config->busy_pin);
    device_gpio_configure_output_with_value("Reset", epaper->config->reset_port, epaper->config->busy_pin, 1);
    device_gpio_configure_output_with_value("DC", epaper->config->dc_port, epaper->config->dc_pin, 0);
    device_gpio_configure_output_with_value("CS", epaper->config->cs_port, epaper->config->cs_pin, 0);

    screen_epaper_set_initializing_phase(epaper);
    //screen_epaper_init_mode(epaper, screen_epaper_FULL);
    return 0;
}

static int screen_epaper_stop(screen_epaper_t *epaper) {
    device_gpio_clear(epaper->config->dc_port, epaper->config->dc_pin);
    device_gpio_clear(epaper->config->cs_port, epaper->config->cs_pin);
    device_gpio_clear(epaper->config->reset_port, epaper->config->reset_pin);
    return 0;
}

static int screen_epaper_pause(screen_epaper_t *epaper) {
    screen_epaper_sleep(epaper);
    return 0;
}

static int screen_epaper_resume(screen_epaper_t *epaper) {
    (void)epaper;
    return CO_ERROR_NO;
}

static ODR_t OD_write_screen_epaper_values_property(OD_stream_t *stream, const void *buf, OD_size_t count,
                                                    OD_size_t *countWritten) {
    screen_epaper_t *epaper = stream->object;
    /* may be unused */ (void)epaper;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static int screen_epaper_phase(screen_epaper_t *epaper) {
    switch (epaper->device->phase) {
    // poll busy pin until and switch to RUNNING when it's clear
    case DEVICE_BUSY:
        return screen_epaper_set_busy_phase(epaper);
    // go through resetting phases which involve 3 separate delays
    case DEVICE_RESETTING:
        return screen_epaper_set_resetting_phase(epaper);
    // clear device initially
    case DEVICE_RUNNING:
        if (epaper->initializing_phase == 0) {

        }
//        if (epaper->initializing_phase == 1) {
//            screen_epaper_clear(epaper);
//        }
        break;
    default:
        break;
    }
    if (epaper->initializing_phase != 0) {
        screen_epaper_set_initializing_phase(epaper);
    } 
    return 0;
}

device_callbacks_t screen_epaper_callbacks = {.validate = screen_epaper_validate,
                                              .construct = (int (*)(void *, device_t *))screen_epaper_construct,
                                              .destruct = (int (*)(void *))screen_epaper_destruct,
                                              .start = (int (*)(void *))screen_epaper_start,
                                              .stop = (int (*)(void *))screen_epaper_stop,
                                              .link = (int (*)(void *))screen_epaper_link,
                                              .pause = (int (*)(void *))screen_epaper_pause,
                                              .resume = (int (*)(void *))screen_epaper_resume,
                                              .phase = (int (*)(void *, device_phase_t phase))screen_epaper_phase,
                                              .write_values = OD_write_screen_epaper_values_property};
