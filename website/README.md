# LOCALPROOF Website

This directory contains the Flask backend and frontend for the LOCALPROOF web app.

---

## Features

- **Interactive Map**: Displays device locations and validation attempts.
- **Validation API**: Validates encrypted QR codes from ESP32 devices.
- **Database**: SQLite database for storing devices and validation logs.

---

## Installation

1. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

2. Run the Flask app:
   ```bash
   cd website
   python app.py
   ```

3. Access the website at `http://localhost:5005`.

---

## Documentation

For more details, refer to the [main project README](../README.md).