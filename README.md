## 🐙 octopodTheHexapod

A **hexapod robot project** built on **Zephyr RTOS** and **ESP32**.  
The goal is a **clean, modular, real-time capable control system** for legged robotics.

## 🐙 Project Origin

The original idea behind this project was to build an **octopod robot**  
(eight legs, inspired by an octopus).

During the design and planning phase, the project evolved:
- mechanical complexity increased
- servo count and power requirements grew significantly
- control and gait planning became more demanding

To keep the project **manageable, reliable, and scalable**, the decision was made
to switch from an octopod to a **hexapod (six-legged) design**.

A hexapod still offers:
- excellent static stability
- multiple gait patterns (tripod, ripple, wave)
- lower mechanical and software complexity

The name **octopodTheHexapod** remains as a reminder of the project’s original idea
and its natural evolution during development.

---

## ✨ Features
- ✅ Zephyr RTOS on ESP32 (dual-core, PRO CPU)
- 🦿 Servo control (direct PWM or PCA9685)
- 🤖 Gait engine (Tripod / Ripple / Wave)
- ⏱️ Real-time scheduling with threads
- 🧭 IMU support (MPU6050 / BNO055 planned)
- 🎮 Control via UART / BLE
- 🔋 Power management

---

## 🧠 Motivation
Arduino-style firmware becomes difficult to maintain for complex robots:
- poor timing guarantees
- monolithic control loops
- limited scalability

**Zephyr RTOS** provides:
- deterministic scheduling
- clear driver abstractions
- scalable, maintainable architecture

This project is designed to fully leverage those strengths.

---

## 🛠️ Hardware
- **MCU:** ESP32 (DevKitC / WROOM-32)
- **Servos:** SG90 / MG996R (typically 18 servos)
- **Servo driver (recommended):** PCA9685 (I²C)
- **IMU (optional):** MPU6050, BNO055
- **Power:** external 5–6 V supply for servos

---

## 🧰 Software Stack
- **RTOS:** Zephyr (≥ 4.x)
- **Toolchain:** Zephyr SDK
- **Build system:** west + CMake + Ninja
- **Language:** C
- **Host OS:** Linux (Debian recommended)


---

## ⚙️ Requirements
```bash
sudo apt install -y \
  git cmake ninja-build python3 python3-venv pipx
````

Install `west`:

```bash
pipx install west
pipx ensurepath
```

---

## 🚀 Build & Flash (ESP32)

### 1️⃣ Prepare Zephyr workspace

```bash
west init ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
west packages pip --install
```

---

### 2️⃣ Build the project

```bash
west build -b esp32_devkitc/esp32/procpu -s ~/octopodTheHexapod -p always
```

---

### 3️⃣ Flash & monitor

```bash
west flash
west espressif monitor
```

---

## 🧪 Current Status

* ✅ Zephyr running on ESP32
* ✅ Toolchain and esptool configured
* ⏳ Servo abstraction in progress
* ⏳ Gait engine planned

---

## 🧭 Roadmap

* [ ] PWM abstraction
* [ ] PCA9685 integration
* [ ] Servo calibration
* [ ] Tripod gait
* [ ] IMU stabilization
* [ ] BLE control
* [ ] Power monitoring

---

## ⚠️ Notes

* **Never power servos from the ESP32**
* Always share **common GND**
* Use real-time thread priorities carefully

---

## 📜 License

MIT License

---

## 👤 Author

**neysi**
Project: *octopodTheHexapod*


## 🖨️ Printables

[3D models and STL files](https://www.printables.com/) (coming soon)
