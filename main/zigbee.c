#include "esp_zigbee_core.h"
#include "esp_check.h"
#include "string.h"


void reportAttribute(uint8_t endpoint, uint16_t clusterID, uint16_t attributeID, void *value, uint8_t value_length)
{
    esp_zb_zcl_report_attr_cmd_t report_attr_cmd = {0};
    report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;
    report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    report_attr_cmd.clusterID = clusterID;
    report_attr_cmd.zcl_basic_cmd.src_endpoint = endpoint;
    // esp_zb_zcl_report_attr_cmd_t cmd = {
    //     .zcl_basic_cmd = {
    //         .dst_addr_u.addr_short = 0x0000,
    //         .dst_endpoint = endpoint,
    //         .src_endpoint = endpoint,
    //     },
    //     .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
    //     .attributeID = attributeID,
    //     .clusterID = clusterID,
    //     .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    // };
    // esp_zb_zcl_attr_t *value_r = esp_zb_zcl_get_attribute(endpoint, clusterID, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, attributeID);
    // memcpy(value_r->data_p, value, value_length);
    ESP_ERROR_CHECK(esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd));
}