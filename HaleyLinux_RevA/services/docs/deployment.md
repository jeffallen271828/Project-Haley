# Haley Linux Host Deployment Guide

## 1. Purpose

This guide describes how to develop the Linux host on a Windows workstation and later move the same system to a dedicated Ubuntu mini PC.

---

## 2. Development Targets

### Temporary Development Target
- Windows workstation
- WSL2 Ubuntu
- VS Code Remote WSL

### Final Deployment Target
- Beelink GTi13 Ultra
- Ubuntu Linux
- local AI services
- Ethernet link to STM32 controller

---

## 3. Recommended Environment Setup

### 3.1 Windows Host
Install:
- Visual Studio Code
- WSL2
- Ubuntu inside WSL2
- Git

### 3.2 Linux Tools
Inside Ubuntu install:
- Python 3.11+
- python3-venv
- pip
- git
- build tools as needed
- sound utilities if audio testing is local

### 3.3 VS Code Extensions
Recommended:
- Python
- Remote - WSL
- YAML
- GitLens
- Docker, later if needed

---

## 4. Repository Layout

Recommended project root:

```text
HaleyLinux_RevA/
├── venv/
├── services/
├── runtime/
├── models/
├── configs/
├── prompts/
├── data/
├── tests/
├── tools/
├── docs/
├── requirements.txt
├── README.md
└── .gitignore
```

---

## 5. Virtual Environment Setup

Create the venv in the project root.

```bash
python3 -m venv venv
source venv/bin/activate
```

The venv contains:
- Python interpreter isolation
- package isolation
- repeatable dependency management

Do not place application source inside the venv directory.

---

## 6. Initial Package Installation

Start with a minimal Python dependency set:
- numpy
- sounddevice or PyAudio
- pyyaml
- fastapi
- uvicorn
- websockets

Add model-specific dependencies later for Whisper.cpp, XTTS, and orchestration helpers.

---

## 7. Development Phases

### Phase 1: Runtime Skeleton
Goals:
- build `app_main.py`
- build the event bus
- build the lifecycle manager
- build the service registry
- build the system monitor

Acceptance:
- application starts
- services are registered
- logs are visible
- monitor snapshots are generated

### Phase 2: Audio and STT
Goals:
- audio capture service
- Whisper.cpp wrapper
- transcript handoff

Acceptance:
- audio enters the pipeline
- speech is converted to text

### Phase 3: LLM and Personality
Goals:
- local LLM runtime
- Haley personality shaping
- conversation orchestration

Acceptance:
- the system generates style-consistent responses

### Phase 4: Memory and TTS
Goals:
- SQLite memory
- memory retrieval
- XTTS output

Acceptance:
- the system remembers useful details
- speech output is produced in the custom voice

### Phase 5: STM32 Integration
Goals:
- connect to embedded controller
- send commands
- receive telemetry

Acceptance:
- stable round-trip communication
- reconnect handling
- health reporting

---

## 8. Windows Simulation Workflow

The Linux host is first simulated on Windows through WSL2 so the actual Linux code path is exercised before deployment.

### Why this works
- the runtime is Linux
- package management is Linux
- shell behavior is Linux
- Python runtime behavior is Linux
- migration to Ubuntu is low friction

### Recommended practice
- run all host-side services in WSL2
- keep the same source tree for final deployment
- avoid Windows-specific runtime assumptions

---

## 9. Deployment to the Beelink PC

When the mini PC arrives:
1. install Ubuntu
2. copy the same source tree
3. recreate the venv
4. install dependencies
5. transfer model assets
6. validate local service startup
7. validate STM32 network communication
8. run long-duration health tests

---

## 10. Deployment Checks

Before considering the Linux host operational, verify:
- service startup order is correct
- logs are readable
- audio path is functional
- model inference works
- TTS output plays correctly
- memory writes and retrieval work
- STM32 communication is stable
- crash recovery is acceptable
- service restart behavior is correct

---

## 11. Operational Notes

The Linux host should be treated as a service-based runtime, not a single script.

Recommended launch pattern:
- start runtime
- register services
- start event bus
- start lifecycle manager
- run orchestration loop
- keep health telemetry active

---

## 12. Long-Term Direction

Once the basic system is stable, the Linux host can be expanded with:
- more advanced memory retrieval
- richer emotional state handling
- audio pre-processing
- optional vision services
- additional embedded nodes
- containerized deployment if needed
