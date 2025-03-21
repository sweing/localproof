### Building Proof of Presence: A First Microcontroller Project

Today’s political challenges—centralized power, bureaucratic inertia, and the erosion of accountability—are magnified in large-scale systems that struggle to adapt to local realities. Yet at the community level, democratic processes often flourish. Here, engagement is more direct, governance is nimbler, and solutions can be tailored to specific needs. This contrast isn’t just theoretical; it’s a call to rethink how we design systems of trust and participation. Decentralized, localized models aren’t just alternatives—they’re safeguards. By empowering communities to govern themselves, they counterbalance the authoritarian tendencies that thrive in overly centralized structures.  

This is where technologies like **proof of presence (PoP)** and **proof of location (PoL)** become critical. PoP verifies that someone or something exists at a specific moment, while PoL confirms *where* they are. Together, they form a framework for building systems rooted in physical reality—systems where trust isn’t abstract but anchored in verifiable, real-world actions. Imagine a local voting mechanism that requires attendees to prove they’re physically present at a town hall, or a resource-sharing network that prioritizes residents within a neighborhood. The potential is vast, but existing projects often prioritize theory over practicality, leaving communities with tools that are more aspirational than usable.  

That’s why this project focuses on building a tangible, open-source device: a microcontroller-based system designed to generate and display time-based one-time passwords (TOTP) as QR codes. But its purpose goes beyond mere functionality. The device is a **platform**, not a product—a foundation communities can adapt, extend, and repurpose. Whether for secure local voting, validating participation in community initiatives, or managing access to shared resources, the hardware and software are intentionally modular. By open-sourcing both the design and code, the goal is to create a starting point, not an endpoint. Communities can iterate on it, integrating features like biometric checks, environmental sensors, or even blockchain-based auditing, depending on their unique needs.  

In doing so, the project aims to advance PoP technology in a way that’s both practical and *political*. By putting adaptable tools directly into the hands of communities, it challenges the concentration of power in distant, opaque systems. It’s a small but deliberate step toward resilience—one QR code at a time.  

---

### The Idea and its Problems

This project started with a deceptively simple question: *How do you prove someone—or something—is truly* ***here***, *right now*? I wanted to build a device that could answer that question in two ways: by verifying **presence** (you exist in this moment) and **location** (you’re exactly where you claim to be). The solution? A small, self-contained gadget that generates time-based one-time passwords (TOTP) and displays them as QR codes—a digital heartbeat that pulses every 30 seconds.  

Inspiration struck when I discovered [Skiply](https://www.skiply.eu/en/ubiqod-key-2/), a company making sleek devices for secure QR-based verification. But while their work impressed me, I saw an opportunity to democratize access. What if instead of a proprietary product, we had an open-source blueprint—a device anyone could build, modify, or integrate into larger systems? That’s the ethos here: no black boxes, no locked firmware. Just transparent tools for communities to reclaim control over verification.  

Generating TOTP codes is the easy part. The real challenge? Ensuring those codes can’t be gamed. A 30-second validity window forces physical proximity—you need to scan the QR code live, in person—but that’s not foolproof. Imagine a bad actor snapping a photo of the code and sending it to an accomplice elsewhere. Suddenly, the “proof” of presence becomes a lie. This “relay attack” loophole undermines the entire system.  

One countermeasure is a server-side cooldown: after a successful scan, the device “locks” for a set period, preventing rapid reuse. If an attacker tries to relay a code, they’d hit a wall—the system would reject duplicates until the cooldown expires. It’s not perfect, but it’s a start. More importantly, it’s a challenge thrown open to the community: *How would you improve this?* By open-sourcing both the hardware and software, the project invites collaborators to iterate—adding biometric checks, environmental sensors, or novel encryption—to turn a prototype into a platform.  

---

### The Hardware Setup

![Device Setup](pictures/device.jpeg?raw=true)

The first challenge was to ensure the device could keep accurate time without relying on an internet connection. I started with an ESP8266 microcontroller and its built-in Wi-Fi module, but I wanted the device to work completely offline. A friend lent me a GPS module, and I discovered that it could also receive accurate time data from GPS satellites.

The hardware setup:

- **ESP32 Microcontroller**: Upgrade from the ESP8266 because I needed more GPIO pins.
- **NEO-6M GPS Module**: To acquire location and time data.
- **DS3231 Real-Time Clock (RTC) Module**: To maintain accurate time.
- **Waveshare 1.54" Black/White E-Paper Display**: I chose a black-and-white display over a color one because the refresh rate is much faster (2 seconds vs. 8 seconds).
- **2N2222 NPN Bipolar Transistor**: To switch off the GPS module once it acquires the location and time.
- **Breadboard and Cables**: For prototyping.
- **5.1K Resistor**: For circuit stability.

---

### Operation breakdown

1. **Boot-Up**: On startup, the device begins acquiring a GPS signal to get the current time and location. This process usually takes up to 2 minutes.
2. **GPS Shutdown**: Once the GPS module has acquired GPS data, it’s turned off using the 2N2222 transistor.
3. **TOTP Generation**: The device waits until the next half-minute mark, then generates a TOTP number using a hard-coded key. 
4. **QR Code Creation**: A link is generated containing the device ID and the encrypted TOTP number. In the future, I want to add the location data as well (which also would have to be encoded).
5. **Display**: The QR code is displayed on the e-paper screen. To make it easier for users to verify the device, I added a small square in the center of the screen showing the first two and last two digits of the TOTP number.
6. **Deep Sleep**: After rendering the QR code, the device goes into deep sleep to save power (27 seconds).

![QR Code Display](pictures/screen.jpeg?raw=true)

I set up a Flask server to verify the QR codes. When a code is scanned, the server returns a JSON response confirming the validity.

At this stage, the device is fully functional. It can generate and display QR codes, and the Flask server can verify them.

![Verification JSON](pictures/verification.jpeg)

---

### Next Steps

1. **Hardware Security Module (HSM)**: I’ve ordered an HSM module to handle TOTP creation and encryption, which will prevent device duplicates.
2. **Web Implementation**: More user-friendly web interface for device registration and verification.
4. **Battery Module**: Adding a battery will make the device portable.
5. **Map Integration**: A map to visualize registered devices and their locations.
6. **Casing Design**: Designing a 3D-printed casing

If you’re interested in following this project, feel free to reach out or leave a comment. I’ll be sharing updates of my progress!

[Project on GitHub](https://github.com/sweing/TOTP-QR-Gen?raw=true)


---