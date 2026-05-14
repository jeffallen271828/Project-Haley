/**
 * @file ai_host_commands.h
 * @brief High-level commands sent from the STM32 to the Linux AI Host.
 *
 * Provides a functional API for RTOS tasks to trigger actions on the
 * Linux AI side without needing to manually format key-value strings.
 */

#ifndef INC_AI_HOST_AI_HOST_COMMANDS_H_
#define INC_AI_HOST_AI_HOST_COMMANDS_H_

#include <stdint.h>
#include "protocol/protocol_types.h"

/**
 * @brief Sends current system telemetry (voltage, temp, etc.) to the AI Host.
 * @return HALEY_PROTO_OK on successful dispatch.
 */
haley_proto_err_t AI_CMD_ReportTelemetry(void);

/**
 * @brief Streams a single frame of raw audio to the AI Host.
 * @param samples Pointer to the 16-bit PCM buffer.
 * @param count   Number of samples in the frame.
 * @return HALEY_PROTO_OK on success.
 */
haley_proto_err_t AI_CMD_SendAudioFrame(const int16_t *samples, uint32_t count);

/**
 * @brief Reports a hardware-level fault to the AI for verbal notification.
 * @param fault_code Bitmask of active faults from protocol_constants.h.
 */
haley_proto_err_t AI_CMD_ReportHardwareFault(uint32_t fault_code);

/**
 * @brief Requests the AI Host to stop speaking or interrupt the current TTS.
 */
haley_proto_err_t AI_CMD_RequestSpeechStop(void);

#endif /* INC_AI_HOST_AI_HOST_COMMANDS_H_ */
