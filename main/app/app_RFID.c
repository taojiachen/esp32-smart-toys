#include <esp_log.h>
#include <esp_check.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "picc/rc522_mifare.h"

static const char *TAG = "rc522-read-write-example";

#define RC522_SPI_BUS_GPIO_MISO (19)
#define RC522_SPI_BUS_GPIO_MOSI (17)
#define RC522_SPI_BUS_GPIO_SCLK (18)
#define RC522_SPI_SCANNER_GPIO_SDA (20)
#define RC522_SCANNER_GPIO_RST (8) // soft-reset

extern struct Nearest_Task
{
    char *key;
    char *datavalue;
    char *starttime;
    long keeptime;
} Nearest_Task;

int flag = 0;

static rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};

static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE)
    {
        rc522_picc_print(picc);
        char uid[RC522_PICC_UID_STR_BUFFER_SIZE_MAX];
        rc522_picc_uid_to_str(&picc->uid, uid, sizeof(uid));
        ESP_LOGE(TAG, "RFID卡UID: %s", uid);
        ESP_LOGE(TAG, "datavalue: %s", Nearest_Task.datavalue);
        if (!strcmp(Nearest_Task.datavalue, uid))
        {
            ESP_LOGE(TAG, "卡号对接成功");
            flag = 1;
        }
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE)
    {
        ESP_LOGI(TAG, "Card has been removed");
        flag = 0;
    }
}

void spi_bus_init()
{
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);
    rc522_config_t scanner_config = {
        .driver = driver,
    };

    rc522_create(&scanner_config, &scanner);
}

void RFID_stop()
{
    rc522_pause(scanner);
}

void RFID_start()
{
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL);
    rc522_start(scanner);
}
