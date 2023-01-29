#define EXAMPLE_ESP_WIFI_SSID                   "NETIASPOT-B70220"
#define EXAMPLE_ESP_WIFI_PASS                   "t36xmzwoz783"
#define EXAMPLE_ESP_MAXIMUM_RETRY               3

#define WIFI_CONNECTED_BIT                      0x00000001 // BIT0
#define WIFI_FAIL_BIT                           0x00000002 // BIT1

static const char *TAG_WIFI = "WIFI STA";

static EventGroupHandle_t s_wifi_event_group;

// Retries counter
static int s_retry_num = 0;

static void Wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG_WIFI, "Connecting to AP...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI, "Retry to connect to AP");
        }
        else
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
}

static void IP_event_handler(void* arg, esp_event_base_t event_base, 
                             int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Connect to wifi and return result
void ConnectWifi(void)
{   
    // Initialize esp network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize default esp event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create wifi station in wifi driver
    esp_netif_create_default_wifi_sta();

    // Setup wifi station with default wifi configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_event_group  = xEventGroupCreate();

    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &Wifi_event_handler,
                                               NULL,
                                               &wifi_handler_event_instance));

    esp_event_handler_instance_t ip_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &IP_event_handler,
                                               NULL,
                                               &ip_handler_event_instance));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // Set wifi controller to station mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Set wifi config 
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    //Start wifi driver
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG_WIFI, "Connected to AP SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGI(TAG_WIFI, "Failed to connect to AP SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } 
    else 
        ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_handler_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    vEventGroupDelete(s_wifi_event_group);

}