#ifndef INC_ADC
#define INC_ADC

#ifdef __cplusplus
extern "C" {
#endif

#include <core/device.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>

#if defined(STMF1)
    #define ADC_CALIBRATION_ENABLED
    #define ADC_CALIBRATION_DELAY 100000
#else
    #define ADC_CALIBRATION_DELAY 0
#endif

#define DEVICE_ADC_MAX_CHANNELS 16

/* Start of autogenerated OD types */
/* 0x6300: Module ADC 1
   ADC Unit used for high-volume sampling of analog signals */
typedef struct module_adc_properties {
    uint8_t parameter_count;
    uint8_t interval;
    uint16_t sample_count_per_channel;
    uint8_t dma_unit;
    uint8_t dma_stream;
    uint8_t dma_channel;
    uint8_t phase;
} module_adc_properties_t;
/* End of autogenerated OD types */


struct module_adc {
    device_t *device;
    module_adc_properties_t *properties;

    uint32_t clock;
    uint32_t address;
    uint32_t dma_address;

    device_t *subscribers[DEVICE_ADC_MAX_CHANNELS];
    size_t channel_count;
    size_t sample_buffer_size;

    uint16_t measurements_per_second;
    uint16_t measurement_counter;
    uint8_t *channels;
    uint16_t *sample_buffer;
    uint32_t *accumulators;
    uint32_t *values;

    uint32_t startup_delay;
} ;

extern device_methods_t module_adc_methods;

uint8_t adc_get_adc_channel(module_adc_t *adc);
uint8_t adc_get_adc_unit(module_adc_t *adc);
void adc_gpio_setup(module_adc_t *adc);
void adc_write(module_adc_t *adc);


void adc_setup(module_adc_t *adc);
void adc_dma_setup(module_adc_t *adc);

size_t adc_integrate_samples(module_adc_t *adc);
void adc_channels_push(module_adc_t *adc, size_t channel);
void adc_channels_alloc(module_adc_t *adc, size_t channel_count);
void adc_channels_free(module_adc_t *adc);

/* Start of autogenerated OD accessors */
typedef enum module_adc_properties_properties {
  MODULE_ADC_INTERVAL = 0x1,
  MODULE_ADC_SAMPLE_COUNT_PER_CHANNEL = 0x2,
  MODULE_ADC_DMA_UNIT = 0x3,
  MODULE_ADC_DMA_STREAM = 0x4,
  MODULE_ADC_DMA_CHANNEL = 0x5,
  MODULE_ADC_PHASE = 0x6
} module_adc_properties_properties_t;

OD_ACCESSORS(module, adc, properties, phase, MODULE_ADC_PHASE, uint8_t, u8) /* 0x63XX06: {"description":null,"label":null} */
/* End of autogenerated OD accessors */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_IDENTIFICATORS_H */

