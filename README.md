# LOCALPROOF

**Proof of Presence and Location Using ESP32 Devices**

LOCALPROOF is a system that uses ESP32 microcontrollers to generate cryptographic proof of presence and location. It combines hardware (ESP32), backend (Flask), and frontend (Leaflet.js) to create a seamless experience for tracking and validating device locations.

---

## Features

- **ESP32 Integration**: Devices generate encrypted QR codes with GPS coordinates and TOTP codes.
- **Real-Time Map**: Interactive map showing device locations and validation attempts.
- **Secure Validation**: AES-256 encryption and TOTP-based validation for tamper-proof data.
- **Scalable Backend**: Flask-powered API with SQLite database for storing devices and logs.

---

## Repository Structure

- **`esp32/`**: Code for ESP32 devices and Python-based simulators.
- **`website/`**: Flask backend and frontend for the LOCALPROOF web app.
- **`README.md`**: This file.
- **`LICENSE`**: Project license.


```
localproof/
├── esp32/                     # ESP32 code and related files
│   ├── esp32_code.ino         # Main ESP32 code
│   ├── python_generator.py    # Python code to simulate ESP32 behavior
│   ├── static/                # Folder to store generated QRs
│   └── README.md              # ESP32-specific documentation
├── website/                   # Flask website code
│   ├── app.py                 # Flask application
│   ├── create_database.py     # Python script to generate database (run once)
│   ├── landing_page.html      # Landing page of LOCALPROOF
│   ├── static/                # Static files (CSS, JS, images)
│   ├── templates/             # HTML templates
│   └── README.md              # Website-specific documentation
├── pictures/                  # Photos for BLOGPOST.md
├── README.md                  # Main project README
├── BLOGPOST.md                # Blog post
├── requirements.txt           # Python dependencies
└── LICENSE                    # Project license (e.g., MIT)
```

---

## Getting Started

### Prerequisites

- Python 3.8+
- Flask
- SQLite
- ESP32 development environment (Arduino IDE or PlatformIO)

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/localproof.git
   cd localproof
   ```
2. Setting up virtual environment & install requirements:
   ```bash
   python -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```

3. Create database and run the Flask app:
   ```bash
   cd website
   python create_database.py
   python app.py
   ```

4. Access the website at `http://localhost:5005`.

---

## ESP32 Code

The ESP32 code and simulators are located in the `esp32/` directory. Refer to the [ESP32 README](esp32/README.md) for details on setting up and running the code.

---

## Contributing

Contributions are welcome!

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---