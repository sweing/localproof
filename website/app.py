from flask import Flask, request, jsonify, render_template, redirect, url_for, flash
from flask_login import LoginManager, UserMixin, login_user, login_required, logout_user, current_user
import sqlite3
import pyotp
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
import base64
import datetime
import bcrypt

app = Flask(__name__)
app.secret_key = 'your_secret_key'  # Change this to a secure key

# Flask-Login setup
login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = 'login'

# User class for Flask-Login
class User(UserMixin):
    def __init__(self, id, username):
        self.id = id
        self.username = username

# Database connection helper function
def get_db_connection():
    conn = sqlite3.connect('database.db')
    conn.row_factory = sqlite3.Row
    return conn

def decrypt_totp(key, cipher_text):
    key = key.encode('utf-8').ljust(32, b'\0')[:32]
    iv_and_ciphertext = base64.urlsafe_b64decode(cipher_text)
    iv = iv_and_ciphertext[:16]
    ciphertext = iv_and_ciphertext[16:]
    cipher = AES.new(key, AES.MODE_CBC, iv)
    plain_text = unpad(cipher.decrypt(ciphertext), AES.block_size)
    return plain_text.decode('utf-8')

# Hash a password
def hash_password(password):
    return bcrypt.hashpw(password.encode('utf-8'), bcrypt.gensalt())

# Verify a password
def verify_password(password, hashed_password):
    return bcrypt.checkpw(password.encode('utf-8'), hashed_password)

# Load user for Flask-Login
@login_manager.user_loader
def load_user(user_id):
    conn = get_db_connection()
    user = conn.execute('SELECT * FROM users WHERE id = ?', (user_id,)).fetchone()
    conn.close()
    if user:
        return User(user['id'], user['username'])
    return None

# Routes for login, register, and logout
@app.route('/login', methods=['POST'])
def login():
    username = request.form.get('username')
    password = request.form.get('password')
    if not username or not password:
        return jsonify({'success': False, 'error': 'Username and password are required'})
    conn = get_db_connection()
    user = conn.execute('SELECT * FROM users WHERE username = ?', (username,)).fetchone()
    conn.close()
    if user and verify_password(password, user['password']):
        user_obj = User(user['id'], user['username'])
        login_user(user_obj)
        return jsonify({'success': True, 'username': user['username']})
    return jsonify({'success': False, 'error': 'Invalid username or password'})

@app.route('/register', methods=['POST'])
def register():
    username = request.form.get('username')
    password = request.form.get('password')
    if not username or not password or username == "unknown":
        return jsonify({'success': False, 'error': 'Username and password are required'})
    conn = get_db_connection()
    try:
        hashed_password = hash_password(password)
        conn.execute('INSERT INTO users (username, password) VALUES (?, ?)', (username, hashed_password))
        conn.commit()
        conn.close()
        return jsonify({'success': True})
    except sqlite3.IntegrityError:
        conn.close()
        return jsonify({'success': False, 'error': 'Username already exists'})

@app.route('/check-login')
def check_login():
    if current_user.is_authenticated:
        return jsonify({'logged_in': True, 'username': current_user.username})
    return jsonify({'logged_in': False})

@app.route('/logout')
@login_required
def logout():
    logout_user()
    return jsonify({'success': True})

# Update validation logs to include username
def log_validation(device_id, status, reason, lat=None, lng=None):
    username = current_user.username if current_user.is_authenticated else "unknown"
    conn = get_db_connection()
    conn.execute('''
        INSERT INTO validation_logs (timestamp, device_id, status, reason, lat, lng, ip, username)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    ''', (
        datetime.datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S'),
        device_id,
        status,
        reason,
        lat,
        lng,
        request.remote_addr,
        username
    ))
    conn.commit()
    conn.close()

@app.route('/validation-logs/<device_id>', methods=['GET'])
def get_validation_logs(device_id):
    conn = get_db_connection()
    logs = conn.execute('''
        SELECT timestamp, status, reason, username
        FROM validation_logs
        WHERE device_id = ?
        ORDER BY timestamp DESC
    ''', (device_id,)).fetchall()
    conn.close()
    return jsonify([dict(log) for log in logs])

@app.route('/', methods=['GET'])
def show_map():
    conn = get_db_connection()
    devices = conn.execute('SELECT * FROM devices').fetchall()
    conn.close()
    return render_template('map.html', devices=devices)

@app.route('/<device_id>/<data_enc>', methods=['GET'])
def validate_totp(device_id, data_enc):
    conn = get_db_connection()
    devices = conn.execute('SELECT * FROM devices').fetchall()
    device = conn.execute('SELECT * FROM devices WHERE device_id = ?', (device_id,)).fetchone()
    conn.close()

    if not device:
        log_validation(device_id, "failed", "Invalid Device ID")
        return render_template('map.html', devices=devices, status="Invalid Device ID")

    try:
        decrypted_data = decrypt_totp(device['secret'], data_enc)
    except:
        log_validation(device_id, "failed", "Decryption Error")
        return render_template('map.html', devices=devices, status="Invalid Data Encryption")

    try:
        totp_number, esp_lat, esp_lng = decrypted_data.split('|')
        esp_lat = float(esp_lat)
        esp_lng = float(esp_lng)
    except (ValueError, IndexError):
        log_validation(device_id, "failed", "Invalid Data Format")
        return render_template('map.html', devices=devices, status="Invalid Decrypted Data Format")

    totp = pyotp.TOTP(device['secret'])
    if totp.verify(totp_number):
        log_validation(device_id, "success", "Valid TOTP", esp_lat, esp_lng)
        status = "Valid Link"
    else:
        log_validation(device_id, "failed", "Invalid TOTP", esp_lat, esp_lng)
        status = "Invalid Link"

    return render_template('map.html',
                         status=status,
                         device_id=device_id,
                         esp_lat=esp_lat,
                         esp_lng=esp_lng,
                         device_lat=device['lat'],
                         device_lng=device['lng'],
                         devices=devices)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5005, debug=True)