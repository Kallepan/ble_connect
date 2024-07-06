#include "main.hh"

/**
 * @brief initialize the advertisement process and set the advertisement data
 *
 * @return int
 */
static int ble_connect_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /*
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info)
     *     o Advertising tx power
     *     o Device name
     */
    memset(&fields, 0, sizeof(fields));

    /*
     * Advertise two flags:
     *      o Discoverability in forthcoming advertisement (general)
     *      o BLE-only (BR/EDR unsupported)
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    static ble_uuid16_t ble_prox_uuid16 = BLE_UUID16_INIT(BLE_SVC_LINK_LOSS_UUID16);
    fields.uuids16 = &ble_prox_uuid16;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        ESP_LOGE(BLE_PROJ_TAG, "error setting advertisement data; rc=%d", rc);
        return rc;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(ble_connect_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_connect_gap_event_handler, NULL);
    if (rc != 0)
    {
        ESP_LOGE(BLE_PROJ_TAG, "error enabling advertisement; rc=%d", rc);
        return rc;
    }

    return 0;
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
int ble_connect_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed */
        ESP_LOGI(BLE_PROJ_TAG, "connection %s; status=%d ",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status);
        if (event->connect.status != 0)
        {
            /* Connection failed; resume advertising. */
            ble_connect_advertise();
            return 0;
        }

        /* Connection established; print the address of the peer */
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        assert(rc == 0);
        ble_connect_print_conn_desc(&desc);

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Connection terminated; resume advertising */
        ESP_LOGI(BLE_PROJ_TAG, "Disconnected; reason=%d", event->disconnect.reason);
        ble_connect_print_conn_desc(&event->disconnect.conn);
        // Resume Advertise
        ble_connect_advertise();

        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters */
        ESP_LOGI(BLE_PROJ_TAG, "connection updated; status=%d ",
                 event->conn_update.status);

        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        ble_connect_print_conn_desc(&desc);

        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        /* Advertising complete; resume advertising */
        ESP_LOGI(BLE_PROJ_TAG, "Advertising complete; reason=%d", event->adv_complete.reason);

        // Resume Advertise
        ble_connect_advertise();

        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        ESP_LOGI(BLE_PROJ_TAG, "encryption change event; status=%d ",
                 event->enc_change.status);

        // Check if the encryption was successful
        if (event->enc_change.status == 0)
        {
            ESP_LOGI(BLE_PROJ_TAG, "Encryption successful");
            return 0;
        }
        // if unsuccessful, disconnect the connection
        ESP_LOGE(BLE_PROJ_TAG, "Encryption failed");
        ble_gap_terminate(event->enc_change.conn_handle, BLE_ERR_REM_USER_CONN_TERM);

        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX:
        ESP_LOGI(BLE_PROJ_TAG, "notify_tx event; conn_handle=%d attr_handle=%d "
                               "status=%d is_indication=%d",
                 event->notify_tx.conn_handle,
                 event->notify_tx.attr_handle,
                 event->notify_tx.status,
                 event->notify_tx.indication);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(BLE_PROJ_TAG, "subscribe event; conn_handle=%d attr_handle=%d "
                               "reason=%d prevn=%d curn=%d previ=%d curi=%d",
                 event->subscribe.conn_handle,
                 event->subscribe.attr_handle,
                 event->subscribe.reason,
                 event->subscribe.prev_notify,
                 event->subscribe.cur_notify,
                 event->subscribe.prev_indicate,
                 event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(BLE_PROJ_TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                 event->mtu.conn_handle,
                 event->mtu.channel_id,
                 event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
    {
        ESP_LOGI(BLE_PROJ_TAG, "PASSKEY_ACTION_EVENT started with action %d", event->passkey.params.action);
        struct ble_sm_io pkey = {};
        // Temporary key for testing
        int key = 123456;

#ifdef CONFIG_BUTTON_PRESS
        int timeout_seconds = 10;
        if (register_boot_button_press(GPIO_NUM_0, timeout_seconds) == 0)
        {
            ESP_LOGI(BLE_PROJ_TAG, "Button pressed.");
        }
        else
        {
            ESP_LOGI(BLE_PROJ_TAG, "Timeout waiting for button press.");
            // prevent the passkey from being entered
            return 0;
        }
#endif // CONFIG_BUTTON_PRESS

        if (event->passkey.params.action == BLE_SM_IOACT_DISP)
        {
            /* Display the passkey to the user */
            pkey.action = event->passkey.params.action;
            pkey.passkey = key;
            ESP_LOGI(BLE_PROJ_TAG, "Enter passkey %" PRIu32 "on the peer side", pkey.passkey);
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(BLE_PROJ_TAG, "ble_sm_inject_io result: %d", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP)
        {
            /* Accept the numeric comparison */
            pkey.action = event->passkey.params.action;
            pkey.numcmp_accept = key;
            ESP_LOGI(BLE_PROJ_TAG, "Accept the numeric comparison on the peer side");
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(BLE_PROJ_TAG, "ble_sm_inject_io result: %d", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_OOB)
        {
            /* Inject the OOB data */
            static uint8_t tem_oob[16] = {0};
            pkey.action = event->passkey.params.action;
            for (int i = 0; i < 16; i++)
            {
                pkey.oob[i] = tem_oob[i];
            }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(BLE_PROJ_TAG, "ble_sm_inject_io result: %d", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_INPUT)
        {
            /* Inject the passkey */
            ESP_LOGI(BLE_PROJ_TAG, "Enter the passkey through console in this format-> key 123456");
            pkey.action = event->passkey.params.action;
            pkey.passkey = key;
            ESP_LOGI(BLE_PROJ_TAG, "Custom Passkey is not supported in this example, using default passkey %" PRIu32, pkey.passkey);

            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(BLE_PROJ_TAG, "ble_sm_inject_io result: %d", rc);
        }
        return 0;
    }
    case BLE_GAP_EVENT_AUTHORIZE:
        MODLOG_DFLT(INFO, "authorize event: conn_handle=%d attr_handle=%d is_read=%d",
                    event->authorize.conn_handle,
                    event->authorize.attr_handle,
                    event->authorize.is_read);

        /* The default behaviour for the event is to reject authorize request */
        event->authorize.out_response = BLE_GAP_AUTHORIZE_REJECT;
        return 0;

    default:
        return 0;
    }

    return 0;
}

static void ble_connect_on_sync(void)
{
    /* This function will be called when NimBLE and the controller are in sync */
    int rc;

#if CONFIG_RANDOM_ADDRESS
    ble_app_set_addr();
    rc = ble_hs_util_ensure_addr(1);
#else
    rc = ble_hs_util_ensure_addr(0);
#endif
    assert(rc == 0);

    /* Set the advertising parameters */
    rc = ble_hs_id_infer_auto(0, &ble_connect_addr_type);
    if (rc != 0)
    {
        ESP_LOGE(BLE_PROJ_TAG, "error determining address type; rc=%d", rc);
        return;
    }

    /* Print the address */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(ble_connect_addr_type, addr_val, NULL);
    print_addr(addr_val);

    /* Start advertising */
    rc = ble_connect_advertise();
    assert(rc == 0);
}

static void ble_connect_on_reset(int reason)
{
    ESP_LOGI(BLE_PROJ_TAG, "Resetting state; reason=%d", reason);
}

static void ble_connect_host_task(void *param)
{
    ESP_LOGI(BLE_PROJ_TAG, "Starting BLE GATTS AUTH host task");

    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

extern "C" void app_main(void)
{
    ESP_LOGI(BLE_PROJ_TAG, "Starting BLE HID Device");

    esp_err_t err;
    int rc;

    /* Initialize NVS — it is used to store PHY calibration data */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Initialize NimBLE host configuration */
    err = nimble_port_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(BLE_PROJ_TAG, "NimBLE port initialization failed");
        return;
    }

    ble_hs_cfg.sync_cb = ble_connect_on_sync;
    ble_hs_cfg.reset_cb = ble_connect_on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    /* Enable the appropriate bit masks to make sure the keys that are needed are exchanged */
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    // Configure the IO capabilities of the device
    ble_hs_cfg.sm_io_cap = CONFIG_SM_TYPE;

    /* Enable MITM protection */
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_mitm = 1;

    /* Set the device name */
    rc = ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    assert(rc == 0);

    /* Start the task */
    nimble_port_freertos_init(ble_connect_host_task);

    /* XXX Need to have template for store */
    ble_store_config_init();
}