# Haley Linux Host Troubleshooting Guide

## 1. Purpose

This document collects common early-stage problems and how to approach them while developing the Linux host.

The project is still in scaffold stage, so many issues will come from environment setup, dependency management, service startup, or communication boundaries.

---

## 2. General Debugging Strategy

When something fails:
1. identify the failing layer
2. verify the layer below it
3. check logs
4. reduce to a minimal reproducible case
5. confirm configuration and paths
6. test one service at a time

Avoid trying to debug the full stack at once.

---

## 3. WSL2 and Ubuntu Issues

### Symptom: VS Code opens the wrong Python
Likely cause:
- the selected interpreter is not the project venv

Fix:
- open the command palette
- select `Python: Select Interpreter`
- choose the interpreter inside `venv/bin/python`

### Symptom: Linux commands are missing
Likely cause:
- the WSL environment is missing packages

Fix:
- install required packages with apt
- confirm Ubuntu is actually running inside WSL2

### Symptom: repository paths look wrong
Likely cause:
- project files are being edited from Windows instead of inside the Linux workspace

Fix:
- open the folder through the WSL remote window
- keep the active working tree in the Linux filesystem

---

## 4. Virtual Environment Issues

### Symptom: `ModuleNotFoundError`
Likely cause:
- the venv is not activated
- the dependency is not installed in the venv

Fix:
- activate the venv
- install the missing package with pip

### Symptom: wrong Python package version
Likely cause:
- system Python is being used instead of the venv

Fix:
- verify the interpreter path
- reselect the project venv in VS Code

---

## 5. Service Startup Issues

### Symptom: app starts but nothing seems to happen
Likely cause:
- placeholder services are running, but the actual service logic has not been implemented yet

Fix:
- confirm that the scaffold is expected to remain idle
- add logging to the runtime and service layer
- verify service registration and lifecycle order

### Symptom: service start failure
Likely cause:
- dependency ordering problem
- unimplemented start function
- exception during initialization

Fix:
- start with one service at a time
- inspect registry dependency order
- log the exception stack trace

### Symptom: circular dependency error
Likely cause:
- service A depends on service B, and service B depends on service A

Fix:
- break the cycle
- move shared definitions into a lower-level module
- use interfaces or forward declarations conceptually, even in Python

---

## 6. Event Bus Issues

### Symptom: events are not being received
Likely cause:
- no subscriber is registered for the topic
- event bus worker is not running
- the topic string does not match

Fix:
- verify the subscription is present
- check wildcard subscribers
- confirm event publication topic names

### Symptom: event handler crashes
Likely cause:
- callback bug or unhandled exception in subscriber

Fix:
- wrap handler logic with clear logging
- keep event handlers small
- do not perform long blocking work in event callbacks

---

## 7. System Monitor Issues

### Symptom: telemetry is incomplete
Likely cause:
- `psutil` is not installed
- the registry is not populated
- the system is running in a minimal environment

Fix:
- install optional dependencies if needed
- verify registry object wiring
- confirm the monitor is attached to the event bus

### Symptom: health checks are misleading
Likely cause:
- placeholder services always return true or false without real logic

Fix:
- treat scaffold health checks as placeholders
- replace them with real service checks as implementation progresses

---

## 8. Protocol and STM32 Link Issues

### Symptom: Linux host cannot reach STM32
Likely cause:
- Ethernet configuration mismatch
- IP address mismatch
- link not up
- STM32 side not yet fully initialized

Fix:
- verify link lights
- confirm static IP or DHCP strategy
- test socket connectivity from Linux
- verify the STM32 side network stack independently

### Symptom: packet parse errors
Likely cause:
- corrupted packet data
- wrong length field
- version mismatch
- payload alignment bug

Fix:
- log packet headers
- verify CRC before parsing payload
- check sequence and length fields
- compare sender and receiver protocol definitions

### Symptom: heartbeat loss
Likely cause:
- service stall
- network disconnect
- missed scheduling window

Fix:
- implement timeout thresholds
- report missed heartbeat events
- transition to safe fallback mode

---

## 9. Audio and AI Service Issues

### Symptom: audio arrives but Whisper does not transcribe
Likely cause:
- incorrect audio format
- wrong sample rate
- buffer framing issue
- model mismatch

Fix:
- confirm PCM format
- confirm sample rate
- test with a known-good sample file
- isolate Whisper.cpp independently

### Symptom: LLM output is poor or inconsistent
Likely cause:
- weak prompt structure
- missing personality shaping
- memory layer not connected
- wrong model choice

Fix:
- validate the system prompt
- simplify the conversation state
- test with fixed prompts
- verify model runtime independently

### Symptom: TTS voice sounds wrong
Likely cause:
- voice model mismatch
- poor voice sample quality
- bad text normalization

Fix:
- verify the voice dataset
- test with short clean text
- inspect model loading and inference settings

---

## 10. Performance Issues

### Symptom: system feels slow
Likely cause:
- inference bottleneck
- audio buffering delay
- too many blocking operations
- oversized model

Fix:
- profile each service
- run audio, LLM, and TTS independently
- measure latency at each stage
- reduce blocking and unnecessary copying

### Symptom: memory usage keeps increasing
Likely cause:
- buffer leaks
- unbounded logs
- cached model artifacts
- retained conversation history

Fix:
- inspect object growth
- cap memory storage
- rotate logs
- validate service cleanup

---

## 11. Development Habit Recommendations

- keep services small
- log early and often
- test one layer at a time
- use configuration files instead of hard-coded constants
- make failures obvious
- preserve the clean boundary between Linux AI host and STM32 controller

---

## 12. First Things to Check When Stuck

- Is the venv activated?
- Is the correct interpreter selected in VS Code?
- Did the service register successfully?
- Is the event bus running?
- Are the dependencies in the correct order?
- Is the STM32 reachable?
- Is the audio format correct?
- Is the model loaded?
- Is the log output visible?

These checks solve a large portion of first-run problems.
