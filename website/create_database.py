import sqlite3

# Connect to the SQLite database (or create it if it doesn't exist)
conn = sqlite3.connect('database.db')
cursor = conn.cursor()

cursor.execute('''
CREATE TABLE IF NOT EXISTS devices (
    device_id TEXT PRIMARY KEY,
    secret TEXT NOT NULL,
    lat REAL,
    lng REAL,
    max_validations INTEGER,
    username TEXT,
    active BOOLEAN DEFAULT FALSE,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
)
''')

# Create a table for validation logs
cursor.execute('''
CREATE TABLE IF NOT EXISTS validation_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    device_id TEXT NOT NULL,
    status TEXT NOT NULL,
    reason TEXT,
    lat REAL,
    lng REAL,
    ip TEXT,
    username TEXT
)
''')

# Create a table for users
cursor.execute('''
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL
);
''')

# Commit changes and close the connection
conn.commit()
conn.close()