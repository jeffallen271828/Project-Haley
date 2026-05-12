# Haley Linux Host Architecture

## 1. Purpose

The Linux host is the high-level intelligence layer of the Haley system. It runs on a local Linux machine during development and deployment, and it is responsible for the non-deterministic parts of the system:

- speech-to-text
- language model inference
- voice synthesis
- personality shaping
- memory retrieval and summarization
- orchestration of the conversation loop
- communication with the STM32 real-time controller

The Linux host should not directly handle hard real-time I/O. That responsibility remains on the STM32 side.

---

## 2. System Role

The Linux host acts as the "brain glue" between user speech, language reasoning, memory, and synthesized speech output.

### Major responsibilities
- receive or capture audio
- transcribe speech using Whisper.cpp
- generate responses using a local LLM
- apply the Haley personality layer
- retrieve and update memory
- synthesize speech using XTTS
- communicate with the STM32 over a clean packet protocol
- monitor service health and restartability

### Non-responsibilities
- deterministic actuator timing
- watchdog-only safety control
- direct low-level peripheral control
- DMA audio capture
- embedded interrupt handling

---

## 3. High-Level Architecture

The Linux host is organized as a set of cooperating services rather than a monolithic application.

```text
Windows Host
    ↓
WSL2 Ubuntu (development)
    ↓
Linux AI Services
    ├── audio_service
    ├── whisper_service
    ├── llm_service
    ├── tts_service
    ├── memory_service
    ├── personality_service
    ├── stm32_interface
    └── orchestrator
```

Final deployment uses the same service structure on native Ubuntu running on the Beelink mini PC.

---

## 4. Layered Design

### 4.1 Runtime Layer
Owns startup, shutdown, health supervision, logging, and service registration.

Files:
- `runtime/app_main.py`
- `runtime/service_registry.py`
- `runtime/event_bus.py`
- `runtime/lifecycle_manager.py`
- `runtime/system_monitor.py`

### 4.2 Service Layer
Owns the functional AI subsystems.

Files:
- `services/audio_service/*`
- `services/whisper_service/*`
- `services/llm_service/*`
- `services/tts_service/*`
- `services/memory_service/*`
- `services/personality_service/*`
- `services/stm32_interface/*`
- `services/orchestrator/*`

### 4.3 Model Layer
Owns external model assets.

Directories:
- `models/whisper/`
- `models/llm/`
- `models/tts/`
- `models/embeddings/`

### 4.4 Configuration Layer
Owns YAML configuration files.

Directories:
- `configs/`
- `prompts/`

### 4.5 Data Layer
Owns generated data, memory databases, logs, and caches.

Directories:
- `data/`

### 4.6 Tooling and Testing Layer
Owns maintenance scripts, benchmarks, and validation.

Directories:
- `tools/`
- `tests/`
- `docs/`

---

## 5. Service Responsibilities

### 5.1 Audio Service
Captures, buffers, and routes PCM audio for downstream processing. This is the entry point for microphone data in the Linux host architecture.

### 5.2 Whisper Service
Runs Whisper.cpp for transcription. It should support streaming input and return time-aligned or chunked transcripts when possible.

### 5.3 LLM Service
Runs the local language model runtime. This service should generate response text and support prompt construction, token streaming, and model selection.

### 5.4 TTS Service
Runs XTTS or equivalent voice synthesis. It transforms response text into spoken audio using the Haley voice profile.

### 5.5 Memory Service
Stores and retrieves conversation memory, preferences, summaries, and important events.

### 5.6 Personality Service
Applies the Haley character layer. This service is responsible for tone, pacing, emotional modulation, and style shaping.

### 5.7 STM32 Interface
Handles communication with the embedded controller over Ethernet or another clean network transport.

### 5.8 Orchestrator
Coordinates the conversational pipeline. It decides how data flows between services and when a response should be generated or suppressed.

---

## 6. Conversation Flow

A typical interaction follows this pattern:

1. audio is captured or received,
2. speech is transcribed,
3. the orchestrator evaluates context,
4. memory is queried,
5. the personality layer shapes the response,
6. the LLM generates text,
7. XTTS synthesizes voice output,
8. the result is spoken back to the user,
9. any hardware action is sent to the STM32.

---

## 7. Personality Model

Haley should feel:
- relaxed
- warm
- casual
- affectionate
- low-energy by default
- teasing but not cruel
- protective when needed
- attentive to user context

Personality should influence:
- response length
- lexical style
- emotional intensity
- TTS prosody
- memory recall priority

The personality layer should not be only a prompt string. It should be an actual behavioral module with state and rules.

---

## 8. Memory Model

### 8.1 Short-Term Memory
Tracks the active conversation, recent references, and immediate emotional context.

### 8.2 Long-Term Memory
Stores:
- user preferences
- recurring topics
- important events
- emotional associations
- summaries of prior sessions

### 8.3 Storage Strategy
Start with SQLite for structured memory. Add optional vector retrieval later if semantic search becomes necessary.

---

## 9. Communication with STM32

The Linux host and STM32 should communicate using a binary or lightweight socket-based protocol.

The protocol should support:
- commands
- telemetry
- heartbeat messages
- fault notifications
- audio metadata
- versioning
- CRC validation
- reconnect handling

The Linux host should treat the STM32 as a real-time peripheral controller, not as a peer AI processor.

---

## 10. Development and Deployment Strategy

### 10.1 Development on Windows
The Linux host software is first developed on a Windows workstation using WSL2 Ubuntu. This allows testing of Linux-native behavior before the mini PC is purchased.

### 10.2 Migration to the Beelink PC
The same codebase is then moved to the Beelink Linux host with minimal architectural changes.

### 10.3 Final Deployment
The final Linux host runs native Ubuntu and hosts the operational AI stack.

---

## 11. Reliability Principles

- Each service should be restartable independently.
- The orchestrator should survive model or service failures when possible.
- The STM32 interface should never be allowed to destabilize the full system.
- Logging and health checks should be enabled from the beginning.
- Service boundaries should remain explicit and narrow.

---

## 12. Design Goals

The architecture should remain:
- modular
- restartable
- testable
- portable
- low-latency
- easy to debug
- compatible with the embedded controller
- expandable to future models and services
