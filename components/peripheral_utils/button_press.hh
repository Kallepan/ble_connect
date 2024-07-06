#ifndef HH_BUTTON_PRESS_HH
#define HH_BUTTON_PRESS_HH
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CONFIG_WAIT_DELAY 1000

#define INPUT_WAIT_TAG "INPUT_WAIT"

#ifdef __cplusplus
extern "C"
{
#endif

#define BUTTON_PRESS_SUCCESS 0
#define BUTTON_PRESS_TIMEOUT 1

    /**
     * @brief Register boot button press
     *
     * @return int
     *     BUTTON_PRESS_SUCCESS if the button was pressed
     *     BUTTON_PRESS_TIMEOUT if the button was not pressed
     */
    int register_boot_button_press(gpio_num_t gpio_num, int timeout_seconds)
    {
        ESP_LOGI(INPUT_WAIT_TAG, "Waiting for button press...");

        esp_err_t err;

        // Configure the GPIO pin for the button
        esp_rom_gpio_pad_select_gpio(gpio_num);
        err = gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
        if (err != ESP_OK)
        {
            ESP_LOGE(INPUT_WAIT_TAG, "Failed to set GPIO direction: %s", esp_err_to_name(err));
            return BUTTON_PRESS_TIMEOUT;
        }
        err = gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
        if (err != ESP_OK)
        {
            ESP_LOGE(INPUT_WAIT_TAG, "Failed to set GPIO pull mode: %s", esp_err_to_name(err));
            return BUTTON_PRESS_TIMEOUT;
        }

        // Wait for the button press
        int timeout = timeout_seconds * 1000 / CONFIG_WAIT_DELAY;
        for (int i = 0; i < timeout; i++)
        {
            if (i % 10 == 0)
                ESP_LOGI(INPUT_WAIT_TAG, "Waiting for button press... (%d/%d)", i, timeout);

            if (gpio_get_level(gpio_num) == 0)
            {
                ESP_LOGI(INPUT_WAIT_TAG, "Button pressed!");
                return BUTTON_PRESS_SUCCESS;
            }
            vTaskDelay(CONFIG_WAIT_DELAY / portTICK_PERIOD_MS);
        }

        ESP_LOGI(INPUT_WAIT_TAG, "Button press timeout!");
        return BUTTON_PRESS_TIMEOUT;
    }

#ifdef __cplusplus
}
#endif

#endif // HH_BUTTON_PRESS_HH