#include "infrared.h"
#include "adc.h"



/// Map of saved GPIO handles that maps to each infrared sensor
static gpio_E infrared_map[infrared_max] = { 0 };

/// Infrared logs
static infrared_logs_S logs[infrared_max] = { 0 };

// static bool infrared_gpio_to_enum(const gpio_E gpio, infrared_E * ir)
// {
//     bool success = false;

//     for (uint8_t g = 0; g < infrared_max; g++)
//     {
//         if (gpio == infrared_map[g])
//         {
//             *ir = (infrared_E)g;
//             success = true;
//             break;
//         }
//     }

//     return success;
// }

/**
 *  Runs a self test on a sensor by sampling from it multiple times and checking for ADC errors
 *  @param ir : The sensor to test
 *  @returns  : True for pass, false for fail
 */
static bool infrared_self_test(const infrared_E ir)
{
    const uint8_t minimum_value_to_not_fault = 10;
    const uint8_t num_tests = 10;
    uint32_t samples[10] = { 0 };
    uint8_t failures = 0;

    for (uint8_t test = 0; test < num_tests; test++)
    {
        const int32_t sample = acd1_sample(infrared_map[ir]);
        if (sample >= minimum_value_to_not_fault)
        {
            ESP_LOGI("infrared_self_test", "[%d] Infrared sensor self test : %d", ir, samples[test]);
            samples[test] = sample;
        }
        else
        {
            ++failures;
        }
    }

    return (failures == 0);
}

/**
 *  Converts the ADC reading in volts to a distance in centimeters
 *  @param voltage : ADC reading
 *  @returns       : Range in centimeters
 */
static float infrared_apply_linearizing_fx(const float voltage)
{
    /**
     *  Linearized distance from voltage
     *  y = mx + b
     *  voltage = slope * distance_cm + y_intercept
     *  voltage - y_intercept = slope * distance_cm
     *  (voltage - y_intercept) / slope = distance_cm
     *  distance_cm = (voltage - y_intercept) / slope
     */

    // Calibration parameters
    static const float slope = -41.4;
    static const float y_intercept = 4963;

    const float distance_cm = (voltage - y_intercept) / slope;

    return distance_cm;
}

void infrared_initialize(const gpio_E * gpio, bool * functional)
{
    for (infrared_E ir = (infrared_E)0; ir < infrared_max; ir++)
    {
        // Save to map
        infrared_map[ir] = gpio[ir];
     
        // Start off as passed test   
        functional[ir] = true;
     
        // Initialize the ADC channel
        if (!adc1_initialize(infrared_map[ir]))
        {
            ESP_LOGE("infrared_initialize", "Failed to initialize infrared sensor %d", ir);
            functional[ir] = false;
            continue;
        }
        
        // Run self test on the sensor
        if ((functional[ir] = infrared_self_test(ir)) == false)
        {
            ESP_LOGE("infrared_initialize", "Failed self test infrared sensor %d", ir);
        }

        if (functional[ir])
        {
            ESP_LOGI("infrared_initialize", "Successfully initialized infrared sensor %d", ir);
        }

        logs[ir].operational = functional[ir];
    }
}

float infrared_burst_sample(const infrared_E ir, const uint8_t samples, const uint16_t delay_us)
{
    uint32_t average = 0;

    if (adc1_is_initialized(infrared_map[ir]))
    {
        for (uint8_t sample = 0; sample < samples; sample++)
        {
            const int32_t reading = acd1_sample(infrared_map[ir]);
            if (reading >= 0)
            {
                average += reading;
                // ESP_LOGI("infrared_burst_sample", "Sample : %d", reading);
                DELAY_US(delay_us);
            }
#if TESTING
            else
            {
                ESP_LOGE("infrared_burst_sample", "Error reading sample %d", sample);
            }
#endif
        }

        average /= samples;
    }

    const float linearized_averaged_distance_cm = infrared_apply_linearizing_fx(average);

    logs[ir].raw_values = average;
    logs[ir].distances = 0;

    return linearized_averaged_distance_cm;
}

infrared_logs_S * infrared_get_logs(void)
{
    return &logs[0];
}