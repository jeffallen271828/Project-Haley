# STM32 to Linux Host Protocol Specification

## 1. Purpose

This document defines the communication link between the STM32 real-time controller and the Linux AI host.

The protocol is intended to support:
- commands from Linux to STM32
- telemetry from STM32 to Linux
- heartbeat monitoring
- fault reporting
- audio metadata
- version negotiation
- reconnect handling

The protocol should remain lightweight, explicit, and easy to debug.

---

## 2. Design Goals

- binary packet format
- fixed header fields
- CRC validation
- versioned message types
- low parsing overhead
- safe handling of malformed packets
- easy logging
- clear request/response semantics

---

## 3. Transport Layer

Recommended transport:
- Ethernet over RMII
- socket-based communication on the Linux side
- lwIP on the STM32 side

Alternative transports may be added later, but Ethernet should be the primary path.

---

## 4. Packet Structure

Every packet should contain a fixed header followed by an optional payload.

```text
+------------------+
| Header           |
+------------------+
| Payload Length   |
+------------------+
| Message Type     |
+------------------+
| Flags            |
+------------------+
| Sequence Number  |
+------------------+
| CRC32            |
+------------------+
| Payload          |
+------------------+
```

---

## 5. Suggested Header Fields

### Message Type
Defines the meaning of the packet:
- command
- telemetry
- heartbeat
- fault
- audio metadata
- version negotiation
- acknowledgement

### Flags
Used for:
- request/response behavior
- reliability hints
- emergency state markers
- optional compression
- reserved future use

### Sequence Number
Used to track ordering, missed packets, and acknowledgements.

### CRC32
Used to detect corruption in the packet data.

---

## 6. Message Categories

### 6.1 Command Messages
Sent from Linux to STM32.

Examples:
- set LED state
- update mode
- request telemetry
- trigger actuator action
- reset a subsystem

### 6.2 Telemetry Messages
Sent from STM32 to Linux.

Examples:
- sensor readings
- state updates
- link health
- power status
- fault status

### 6.3 Heartbeat Messages
Sent periodically by both sides to verify liveness.

### 6.4 Fault Messages
Used when a subsystem enters a failure state or needs recovery.

### 6.5 Audio Metadata Messages
Used to describe audio chunks, timestamps, format, or capture state.

### 6.6 Version Negotiation Messages
Used at link startup to verify protocol compatibility.

---

## 7. Message Flow

### Startup Flow
1. Linux host starts.
2. STM32 boots and enters safe state.
3. Heartbeat exchange begins.
4. Version negotiation occurs.
5. Command channel becomes active.
6. Telemetry begins streaming.

### Normal Operation
1. Linux issues commands.
2. STM32 executes deterministic actions.
3. STM32 reports telemetry.
4. Linux updates state and conversation context.

### Fault Handling
1. One side detects link failure or invalid data.
2. Fault packet is emitted if possible.
3. Safe fallback behavior is enabled.
4. Heartbeat and reconnect logic attempt recovery.

---

## 8. Error Handling

The protocol should reject:
- malformed headers
- invalid CRC values
- unsupported versions
- oversized payloads
- unexpected message types

Invalid packets should not crash the receiving side.

---

## 9. Recommended Linux-Side Responsibilities

Linux host should:
- serialize outgoing packets
- parse incoming packets
- validate CRC
- track sequence numbers
- maintain connection state
- retry or reconnect when needed

---

## 10. Recommended STM32-Side Responsibilities

STM32 should:
- validate packet integrity
- execute only safe and allowed commands
- report telemetry
- maintain watchdog-safe behavior
- preserve deterministic timing

---

## 11. Example Message Types

- `0x01` heartbeat
- `0x02` command
- `0x03` acknowledgement
- `0x04` telemetry
- `0x05` fault
- `0x06` audio metadata
- `0x07` version negotiation

These values may be changed later, but the table should remain stable once implementation begins.

---

## 12. Implementation Notes

- Use binary structures, not JSON, for runtime packets.
- Keep packet parsing strict.
- Keep payload handling separate from transport handling.
- Add logging for all invalid packet conditions.
- Do not allow the AI layer to bypass command validation on the STM32 side.

---

## 13. Future Extensions

The protocol can later support:
- more embedded nodes
- streaming audio frames
- remote diagnostics
- additional sensor types
- optional compression
- secure authentication if required
