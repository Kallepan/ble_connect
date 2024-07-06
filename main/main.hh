#ifndef _MAIN_HH_
#define _MAIN_HH_

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h" // For tasks

/* NimBLE Stack */
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/prox/ble_svc_prox.h"
#include "host/util/util.h"
#include "host/ble_hs.h"

#define CONFIG_DEBUG 1
#define CONFIG_RANDOM_ADDRESS 1
#define CONFIG_BUTTON_PRESS 1
#define CONFIG_SM_TYPE BLE_SM_IO_CAP_KEYBOARD_DISP

#if CONFIG_BUTTON_PRESS
#include "peripheral_utils/button_press.hh"
#endif

#define BLE_PROJ_TAG "BLE_HID"
#define BLE_DEVICE_NAME "ESP32_BLE_HID"

#ifdef __cplusplus
extern "C"
{
#endif

#if CONFIG_RANDOM_ADDRESS
    static uint8_t ble_connect_addr_type = BLE_OWN_ADDR_RANDOM;
    /**
     * @brief Set a random address for the device
     */
    void ble_app_set_addr(void)
    {
        ble_addr_t addr;
        int rc;

        /* Generate a non-resolvable private address. */
        rc = ble_hs_id_gen_rnd(1, &addr);
        assert(rc == 0);

        /* Set the generated address. */
        rc = ble_hs_id_set_rnd(addr.val);
        assert(rc == 0);
    }
#else
static uint8_t ble_connect_addr_type;
#endif

    int ble_connect_gap_event_handler(struct ble_gap_event *event, void *arg);

    /**
     * @brief Print the device address
     */
    void print_addr(const void *addr)
    {
        const uint8_t *u8p;

        u8p = static_cast<const uint8_t *>(addr);
        ESP_LOGI(BLE_PROJ_TAG, "Device Address: %02x:%02x:%02x:%02x:%02x:%02x",
                 u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
    }

    /**
     * @brief Logs information about a connection to the console.
     */
    void ble_connect_print_conn_desc(struct ble_gap_conn_desc *desc)
    {
        ESP_LOGI(BLE_PROJ_TAG, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                 desc->conn_handle, desc->our_ota_addr.type);
        print_addr(desc->our_ota_addr.val);
        ESP_LOGI(BLE_PROJ_TAG, " our_id_addr_type=%d our_id_addr=",
                 desc->our_id_addr.type);
        print_addr(desc->our_id_addr.val);
        ESP_LOGI(BLE_PROJ_TAG, " peer_ota_addr_type=%d peer_ota_addr=",
                 desc->peer_ota_addr.type);
        print_addr(desc->peer_ota_addr.val);
        ESP_LOGI(BLE_PROJ_TAG, " peer_id_addr_type=%d peer_id_addr=",
                 desc->peer_id_addr.type);
        print_addr(desc->peer_id_addr.val);
        ESP_LOGI(BLE_PROJ_TAG, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                               "encrypted=%d authenticated=%d bonded=%d\n",
                 desc->conn_itvl, desc->conn_latency,
                 desc->supervision_timeout,
                 desc->sec_state.encrypted,
                 desc->sec_state.authenticated,
                 desc->sec_state.bonded);
    }

    void ble_store_config_init(void);

#ifdef __cplusplus
}
#endif

#endif // _MAIN_HH_