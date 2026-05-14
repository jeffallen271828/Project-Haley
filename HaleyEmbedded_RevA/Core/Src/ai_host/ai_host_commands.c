/**
 * @file ai_host_commands.c
 * @brief Implementation of AI Host commands routing to Ethernet Protocol.
 */

#include "ai_host_commands.h"
#include "ethernet/ethernet_protocol.h"
#include "cmsis_os.h"

haley_proto_err_t AI_CMD_ReportTelemetry(void) {
    // Note: In a full implementation, you would populate this from your
    // sensor/power management module.
    haley_pay_telem_sensor_t telem = {0};

    // Example mapping to the protocol layer
    eth_proto_result_t res = ETH_PROTO_SendTelemetrySensor(&telem);

    return (res == ETH_PROTO_OK) ? HALEY_PROTO_OK : HALEY_PROTO_ERR_SEND;
}

haley_proto_err_t AI_CMD_SendAudioFrame(const int16_t *samples, uint32_t count) {
    if (samples == NULL) return HALEY_PROTO_ERR_NULL;

    // Convert sample count to byte length (16-bit PCM = 2 bytes per sample)
    uint16_t data_len = (uint16_t)(count * 2);

    eth_proto_result_t res = ETH_PROTO_SendAudioData((const uint8_t*)samples, data_len);

    return (res == ETH_PROTO_OK) ? HALEY_PROTO_OK : HALEY_PROTO_ERR_SEND;
}

haley_proto_err_t AI_CMD_ReportHardwareFault(uint32_t fault_code) {
    // Protocol layer expects 8-bit flags and a timestamp
    uint8_t flags = (uint8_t)(fault_code & 0xFF);
    uint32_t uptime = osKernelGetTickCount();

    eth_proto_result_t res = ETH_PROTO_SendFaultNotification(flags, uptime);

    return (res == ETH_PROTO_OK) ? HALEY_PROTO_OK : HALEY_PROTO_ERR_SEND;
}

haley_proto_err_t AI_CMD_RequestSpeechStop(void) {
    /* * Since ethernet_protocol.h does not have a dedicated StopSpeech function,
     * we utilize a System Status packet to signal the state change to the AI Host,
     * or a generic command if your protocol_types defines one.
     */
    haley_pay_status_sys_t status = {0};
    // Logic here would set a 'stop' flag in the status payload if supported

    eth_proto_result_t res = ETH_PROTO_SendSystemStatus(&status);

    return (res == ETH_PROTO_OK) ? HALEY_PROTO_OK : HALEY_PROTO_ERR_SEND;
}
