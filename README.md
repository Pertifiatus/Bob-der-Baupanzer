<h1 align="center">
  <br>
  🛠️ BOB DER BAUPANZER
  <br>
</h1>

<h3 align="center">
  <em>A remote-controlled tracked vehicle with a robotic arm</em>
</h3>

<p align="center">
  <img src="docs/images/build-complete.jpg" alt="Bob der Baupanzer, complete build" width="600"/>
</p>

---

## What is Bob?

**Bob der Baupanzer** is a DIY robotics project: a remote-controlled tracked vehicle with a robotic arm on top. It is controlled over **ExpressLRS (ELRS)** and has **3 analog FPV cameras** that you can switch between, one of them on a head-tracked gimbal.

Bob is a two-year **NWT advanced course (Leistungskurs) project** by **Per, Lasse and Dominic**.

<p align="center">
  <img src="docs/images/sketch-concept.jpg" alt="First concept sketch" width="380"/>
  <img src="docs/images/render-concept.jpg" alt="Concept render" width="380"/>
</p>

---

## ✨ Features

<table>
<tr>
<td width="50%">

### ELRS remote control
- RadioMaster Boxer transmitter
- ExpressLRS 2.4 GHz (CRSF protocol)
- Latency < 10 ms
- Failsafe: stops all motors when the signal is lost

</td>
<td width="50%">

### Multi-camera system
- 3 analog FPV cameras
- 3D-printed, head-tracked camera gimbal
- Front, arm and 3rd-person camera
- Real-time video feed

</td>
</tr>
<tr>
<td width="50%">

### Robotic arm
- NEMA 17 stepper motors with 3D-printed cycloidal gearboxes (100:1)
- NEMA 17 and NEMA 14 driven gripper
- Precise positioning with homing

</td>
<td width="50%">

### Tracks
- 3D-printed chain links
- Rigid suspension with large ball-bearing road wheels
- Stepper-driven (NEMA 17, 20:1 gearbox)

</td>
</tr>
</table>

---

## System architecture

```
RadioMaster Boxer ──ELRS──► Receiver ──CRSF──► TX (Seeed S3 SuperMini)
                                                   │
                                                   ▼
                                             ESP32 (main controller)
                                     camera gimbal · video switcher · LEDs
                                                   │
                       ┌───────────────────────────┼───────────────────────────┐
                       ▼                           ▼                           ▼
                 A1 – Motor                  A2 – Greif (arm)            A3 – Zange (gripper)
             Arduino + CNC shield         Arduino + CNC shield         Arduino + CNC shield
```

The TX board decodes the CRSF channels and sends each device its own data packet. If all channels stay at zero (signal lost), the TX stops sending and every Arduino goes into **failsafe** mode instead of letting the motors run away.

### Channel mapping

| Channel | Function | Channel | Function |
|---|---|---|---|
| Ch1 | Throttle | Ch6 | Arm switch: motor |
| Ch2 | Elevator | Ch7 | Arm switch: gripper |
| Ch3 | Yaw | Ch8 | Gripper (Zange) switcher |
| Ch4 | Roll | Ch9 | Video switcher |
| Ch5 | Mode switcher | Ch13–14 | Head tracker |

<p align="center">
  <img src="docs/images/wiring-diagram.jpg" alt="Wiring diagram" width="600"/>
</p>

---

## Hardware

### Electronics overview

<p align="center">
  <img src="https://github.com/user-attachments/assets/86cd4f3f-e648-4e8a-aa2a-8e63ad2b47fa" alt="Electronics bay" width="500"/>
</p>

The electronics are mounted **vertically** in 3D-printed holders so everything fits inside the hull. Cooling comes from 3× 60 mm intake fans and 2× 80 mm exhaust fans.

<p align="center">
  <img src="docs/images/mount-render.jpg" alt="Vertical mount render" width="280"/>
  <img src="docs/images/mount-esp32.jpg" alt="ESP32 mount" width="280"/>
  <img src="docs/images/fan-mount.jpg" alt="Fan mount" width="280"/>
</p>

### Parts list

| Area | Parts |
|---|---|
| **General** | 3× Arduino with CNC shield · 1× ESP32 (main controller) · 1× ESP32 mini (Crossfire receiver) · 1× BetaFPV Nano ELRS TX · 3× 60×25 fan (intake) · 2× 80×25 fan (exhaust) · 2× step-down converter |
| **Robotic arm** | 2× NEMA 17 (large) with 100:1 gearbox · 1× NEMA 17 (large) without gearbox · 2× NEMA 17 (small) · 1× NEMA 14 · 30×45 cm base plate · 28×26×500 mm carbon tube |
| **Drive** | 2× NEMA 17 (large) with 20:1 gearbox |
| **Lighting** | LCD screen · NeoPixel LEDs along the arm · colored COB LED as tail lights · 6× NeoPixel headlights |
| **Camera** | 3-channel video switcher · 3× analog camera · 1× VTX · 2× 9g servos for head tracking · 10-LED strip |

Some parts (endstops, NEMA 17 steppers, cable harnesses, LCD) came from an old MakerBot Replicator 2X that was given away for free and taken apart for spare parts.

### 3D-printed components

- **Chain links:** modular design for easy assembly and quick repairs
- **Video system:** head-tracked servo gimbal
- **Robotic arm:** joints, holders, cycloidal gearboxes, gripper
- **Cable management:** a kind of magnetic Gridfinity for easy wiring

---

## Development log

### Drivetrain
The first driving test showed several problems: the gearbox skipped under high torque, the small road wheels made the tracks jump off, and the flexible pendulum suspension added a lot of friction.
**Fix:** a fully rigid suspension with 4 much larger road wheels on ball bearings. The motor decision (stepper vs. brushed DC vs. BLDC) was evaluated after the test. A stepper's step accuracy doesn't matter for driving, but the steppers were kept as long as their power is enough.

<p align="center">
  <img src="docs/images/tracks-render.jpg" alt="Track render" width="280"/>
  <img src="docs/images/tracks-suspension.jpg" alt="Suspension" width="280"/>
  <img src="docs/images/tracks-wheels.jpg" alt="Road wheels" width="280"/>
</p>

### Camera setup
Goals: video transmission, switching between 3 cameras and a "deck" camera with head tracking. Video switching and the gimbal work; an OSD is still being considered.

<p align="center">
  <img src="docs/images/camera-gimbal.jpg" alt="Camera gimbal" width="280"/>
  <img src="docs/images/camera-gimbal-printed.jpg" alt="Printed gimbal" width="280"/>
</p>

### Receiver & control
Receiving the data, forwarding it to the Arduinos and turning it into motor movements all work.

<p align="center">
  <img src="docs/images/radio-transmitter.jpg" alt="RadioMaster transmitter" width="280"/>
  <img src="docs/images/receiver-test.jpg" alt="Receiver test" width="280"/>
</p>

### Robotic arm & gripper

<p align="center">
  <img src="docs/images/gearbox-cycloid.jpg" alt="Cycloidal gearbox" width="280"/>
  <img src="docs/images/gearbox-test.jpg" alt="Gearbox test" width="280"/>
  <img src="docs/images/gripper-finray.jpg" alt="Fin-ray gripper" width="280"/>
</p>

### Milestones

- [ ] Driving
- [ ] Mount all components securely
- [ ] Move the robotic arm
- [ ] Grip objects

### 🎥 Videos

- [Driving test](https://youtu.be/1AyWPFsoG3s)
- [Servo gimbal](https://youtube.com/shorts/PtDqVKv-9q8)

---

## Repository structure

```
Elektronik/
  Code/TX ESP 3-Arduino/   Current firmware, one folder per device:
    TX/                    Seeed S3 SuperMini: CRSF receiver → packets to ESP32 + Arduinos
    ESP32/                 Main controller: gimbal, video switcher, LEDs
    Motor-A1/              Arduino 1: drive motors
    Greif-A2/              Arduino 2: robotic arm
    Zange-A3/              Arduino 3: gripper
                           (each folder has Changelogs.txt, highest version = latest)
  Code/Einzeln/            Single-component tests (camera, gimbal, LCD, LEDs, CRSF library)
  Code/Alt/                Older experiments
  Pinouts/                 ESP32 pinouts, current tables
  Schaltplan/              Fritzing schematic
Greifarm/3dModels/         Cycloidal gearbox (20:1) and chain STL files
docs/images/               Project photos
```

---

## 📸 Gallery

<p align="center">
  <img src="docs/images/build-chassis.jpg" alt="Chassis" width="280"/>
  <img src="docs/images/build-electronics-grid.jpg" alt="Electronics grid" width="280"/>
  <img src="docs/images/build-electronics-bay.jpg" alt="Electronics bay" width="280"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/f9989d5a-d398-4a78-951f-df0d8d23c0f0" alt="Development setup" width="280"/>
  <img src="https://github.com/user-attachments/assets/d742e3e7-910f-42ba-a250-abf2e0da4194" alt="Gripper detail" width="280"/>
  <img src="https://github.com/user-attachments/assets/d18a3bb0-ae8f-4d5a-83aa-1506f44fccd6" alt="Servo assembly" width="280"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/75b00a04-d41b-44a3-9bb3-3060cca1d023" alt="Test setup" width="280"/>
  <img src="https://github.com/user-attachments/assets/2054ba01-9de9-4f12-8a84-cfdb0529d834" alt="Track drive" width="280"/>
  <img src="https://github.com/user-attachments/assets/d4fde511-e9dd-4638-9a0c-84f0ac590c96" alt="Camera module" width="280"/>
</p>

<p align="center">
  <img src="docs/images/build-vertical-mounts.jpg" alt="Vertical mounts" width="280"/>
  <img src="docs/images/render-idea.jpg" alt="Idea render" width="280"/>
</p>

---

<p align="center"><em>Made by Per, Lasse & Dominic</em></p>
