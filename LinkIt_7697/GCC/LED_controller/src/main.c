/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sys_init.h"
#include "wifi_lwip_helper.h"
#include "wifi_api.h"
#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif
#include "bsp_gpio_ept_config.h"
#include "task_def.h"
#include "syslog.h"
#include "os.h"
#include <nvdm.h>

#include "lwip/ip4_addr.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"

/* websocket */
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ethernetif.h"
#include "mcs.h"
// #include "bsp_gpio_ept_config.h"
#include "hal_gpio.h"

#include "cJSON.h"


// Input your Wi-Fi setting here 
#define SSID "mcs"
#define PASSWORD "mcs1234"

//Input MCS Lite websocket server here
#define WEBSOCKET_SERVER "192.168.1.241"
#define WEBSOCKET_PORT 8000

//Input MCS Lite RESTful server here
#define API_SERVER "192.168.1.241"
#define API_PORT 3000

// Input MCS Lite device ID and Key here
#define DEVICE_ID "Bk1chnjbW"
#define DEVICE_KEY "0676619c94374d542fc8421d0ed73ad3e189c03459453c214a8276cb26c341db"

#define APP_TASK_NAME                   "MCS Lite LED Sample"
#define APP_TASK_STACKSIZE              (6*1024)
#define APP_TASK_PRIO                   TASK_PRIORITY_NORMAL

#ifndef MTK_DEBUG_LEVEL_NONE
log_create_module(main, PRINT_LEVEL_ERROR);

LOG_CONTROL_BLOCK_DECLARE(main);
LOG_CONTROL_BLOCK_DECLARE(common);
LOG_CONTROL_BLOCK_DECLARE(hal);
LOG_CONTROL_BLOCK_DECLARE(lwip);
LOG_CONTROL_BLOCK_DECLARE(minisupp);
LOG_CONTROL_BLOCK_DECLARE(inband);
LOG_CONTROL_BLOCK_DECLARE(wifi);

log_control_block_t *syslog_control_blocks[] = {
    &LOG_CONTROL_BLOCK_SYMBOL(main),
    &LOG_CONTROL_BLOCK_SYMBOL(common),
    &LOG_CONTROL_BLOCK_SYMBOL(hal),
    &LOG_CONTROL_BLOCK_SYMBOL(lwip),
    &LOG_CONTROL_BLOCK_SYMBOL(minisupp),
    &LOG_CONTROL_BLOCK_SYMBOL(inband),
    &LOG_CONTROL_BLOCK_SYMBOL(wifi),
    NULL
};

static void syslog_config_save(const syslog_config_t *config)
{
    char *syslog_filter_buf;

    syslog_filter_buf = (char*)pvPortMalloc(SYSLOG_FILTER_LEN);
    configASSERT(syslog_filter_buf != NULL);
    syslog_convert_filter_val2str((const log_control_block_t **)config->filters, syslog_filter_buf);
    nvdm_write_data_item("common", "syslog_filters", \
                         NVDM_DATA_ITEM_TYPE_STRING, (const uint8_t *)syslog_filter_buf, strlen(syslog_filter_buf));
    vPortFree(syslog_filter_buf);
}

static uint32_t syslog_config_load(syslog_config_t *config)
{
    uint32_t sz = SYSLOG_FILTER_LEN;
    char *syslog_filter_buf;

    syslog_filter_buf = (char*)pvPortMalloc(SYSLOG_FILTER_LEN);
    configASSERT(syslog_filter_buf != NULL);
    nvdm_read_data_item("common", "syslog_filters", (uint8_t *)syslog_filter_buf, &sz);
    syslog_convert_filter_str2val(config->filters, syslog_filter_buf);
    vPortFree(syslog_filter_buf);

    return 0;
}
#endif

void tcp_callback(char *rcv_buf) {
    /* gpio init */
    int gpio = 36;
    hal_pinmux_set_function(gpio, 8);
    hal_gpio_status_t ret;
    ret = hal_gpio_init(gpio);
    ret = hal_gpio_set_direction(gpio, HAL_GPIO_DIRECTION_OUTPUT);
    
    /* upload data */
    char data_buf [200] = {0};
    strcat(data_buf, "string_display");
    strcat(data_buf, ",,");

    /* parse data */
    char* json = cJSON_Parse(rcv_buf);
    cJSON * datachannelIdObj = cJSON_GetObjectItem(json, "datachannelId");
    char * datachannelId = cJSON_Print(datachannelIdObj);
    cJSON * values = cJSON_GetObjectItem(json, "values");
    cJSON * value = cJSON_GetObjectItem(values, "value");

    if (0 == strcmp(datachannelId, "\"switch_controller\"")) {
        char* switchValue = cJSON_Print(value);
        printf("switchValue: %s\n", switchValue);
        if (0 == strcmp(switchValue, "1")) {
            ret = hal_gpio_set_output(gpio, 1);
    				strcat(data_buf, "LED is ON.");
    				printf("start to send data");
    				mcs_upload_datapoint(data_buf, API_SERVER, API_PORT, DEVICE_ID, DEVICE_KEY);            
        } else {
            ret = hal_gpio_set_output(gpio, 0);
    				strcat(data_buf, "LED is OFF.");
    					printf("start to send data");
    				mcs_upload_datapoint(data_buf, API_SERVER, API_PORT, DEVICE_ID, DEVICE_KEY);               
        }
        ret = hal_gpio_deinit(gpio);
    }
}

static void app_entry(void *args)
{
    lwip_net_ready();
    mcs_tcp_init(tcp_callback, WEBSOCKET_SERVER, WEBSOCKET_PORT, DEVICE_ID, DEVICE_KEY );
    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS); // release CPU
    }
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    /* Do system initialization, eg: hardware, nvdm, logging and random seed. */
    system_init();

#ifndef MTK_DEBUG_LEVEL_NONE
    log_init(syslog_config_save, syslog_config_load, syslog_control_blocks);
#endif

    LOG_I(common, "FreeRTOS Running");

  /* User initial the parameters for wifi initial process,  system will determin which wifi operation mode
     * will be started , and adopt which settings for the specific mode while wifi initial process is running
     */
    wifi_config_t config = {0};
    config.opmode = WIFI_MODE_STA_ONLY;
    strcpy((char *)config.sta_config.ssid, SSID);
    strcpy((char *)config.sta_config.password, PASSWORD);
    config.sta_config.ssid_length = strlen((const char *)config.sta_config.ssid);
    config.sta_config.password_length = strlen((const char *)config.sta_config.password);


    /* Initialize wifi stack and register wifi init complete event handler,
     * notes:  the wifi initial process will be implemented and finished while system task scheduler is running.
     */
    wifi_init(&config, NULL);

    /* Tcpip stack and net interface initialization,  dhcp client, dhcp server process initialization*/
    lwip_network_init(config.opmode);
    lwip_net_start(config.opmode);

    xTaskCreate(app_entry,
        APP_TASK_NAME,
        APP_TASK_STACKSIZE/sizeof(portSTACK_TYPE),
        NULL,
        APP_TASK_PRIO,
        NULL);


    /* Create a user task for demo when and how to use wifi config API  to change WiFI settings,
       Most WiFi APIs must be called in task scheduler, the system will work wrong if called in main(),
       For which API must be called in task, please refer to wifi_api.h or WiFi API reference.
           xTaskCreate(user_wifi_app_entry,
                UNIFY_USR_DEMO_TASK_NAME,
                UNIFY_USR_DEMO_TASK_STACKSIZE / 4,
                NULL, UNIFY_USR_DEMO_TASK_PRIO, NULL);
    */

    /* Initialize cli task to enable user input cli command from uart port.*/
#if defined(MTK_MINICLI_ENABLE)
    cli_def_create();
    cli_task_create();
#endif
    /* Start the scheduler. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for ( ;; );
}
