#!/usr/bin/env python3
import subprocess
import threading
import json
import os
import ipaddress
import signal
# import shlex
import time
import sqlite3
from sqlite3 import Error
from datetime import datetime


# Global variables
node = None
time = None
temp = None
hum = None
pres = None

# Rest of your code...


ssid = "CircuitCraft-WiFi"
password =""

def create_connection(db_file):
    """ Create a database connection to a SQLite database """
    conn = None
    try:
        conn = sqlite3.connect(db_file)
        print(f"Connected to {db_file}")
        return conn
    except Error as e:
        print(e)
    return conn

def create_table(conn):
    """ Create a table to store the data if not exists """
    try:
        cursor = conn.cursor()
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS WeatherData (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                node INTEGER,
                time TEXT,
                temperature REAL,
                humidity REAL,
                pressure REAL
            )
        """)
        print("Table created successfully")
    except Error as e:
        print(e)

def insert_data(conn, node, time, temp, hum, pres):
    """ Insert data into the table """
    try:
        print("ik ben hier")
        cursor = conn.cursor()
        cursor.execute("""
            INSERT INTO WeatherData (node, time, temperature, humidity, pressure)
            VALUES (?, ?, ?, ?, ?)
        """, (node, time, temp, hum, pres))
        conn.commit()
        print("Data inserted successfully")
    except Error as e:
        print(e)

def get_ip_range():
    try:
        result = subprocess.run(["ifconfig"], capture_output=True, text=True)
        output = result.stdout

        interfaces = [line.split(":")[0] for line in output.split("\n") if ":" in line]
        wireless_interface = next((iface for iface in interfaces if "wlan" in iface), None)

        if wireless_interface:
            inet_lines = [line for line in output.split(f"{wireless_interface}:")[-1].split("\n") if "inet" in line]

            if inet_lines:
                inet_line = inet_lines[0]
                current_ip = inet_line.split()[1]
                netmask = inet_line.split()[3]
                print("My Ip =", current_ip)

                network = ipaddress.IPv4Network(f"{current_ip}/{netmask}", strict=False)
                ip_range = f"{network.network_address}/24"

                return ip_range
            else:
                print("No 'inet' line found for the wireless interface.")
                return None
        else:
            print("Wireless interface not found.")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def scan_and_connect(ip_range):
    try:
        subprocess.run(['rm', 'boost_output.txt'], check=True)
    except Exception as e:
            print(e)
    best_strength = -100
    best_ip = None

    nmap_command = f"nmap -p 5555 --open {ip_range}"
    result = subprocess.run(nmap_command, shell=True, capture_output=True, text=True)
    output_lines = result.stdout.split('\n')

    for line in output_lines:
        if "Nmap scan report for" in line:
            ip_address = line.split()[-1]
            print("Found host at", ip_address)

            painlessmesh_command = f"painlessMeshBoost -c {ip_address}"
            print(painlessmesh_command)

            try:
                painlessmesh_command = f"painlessMeshBoost -c {ip_address}"
                process = subprocess.Popen(painlessmesh_command, shell=True, stdout=subprocess.PIPE, text=True)

                with open('boost_output.txt', 'a') as file:
                  for line in process.stdout:
                    kaas=line
               #     print(line, end='')
                    function(line)

                  
                    

               
                    file.write(line)

                last_size = -1
                no_change_counter = 0
                while True:
                    try:
                        current_size = os.path.getsize("boost_output.txt")
                    except OSError:
                        current_size = 0
       
                    if current_size == last_size:
                        no_change_counter += 1
                    else:
                        no_change_counter = 0

                    if no_change_counter >= 6:  # 6*5 seconds = 30 seconds
                        print("No change in file size for 30 seconds")
                        process.send_signal(signal.SIGINT)  # Send an interrupt signal to the process
                        break

                    last_size = current_size
                    time.sleep(5)  # Wait for 5 seconds


            except Exception as e:
                print("Error running subprocess:", e)

        





def function(json_string):
    string="'"+json_string+"'"
    fixed_kaas = string.replace('{', '').replace('}', '').replace('"','').replace('msg:','').replace('\n','')
    if 'event:receive'in fixed_kaas:
#        print(fixed_kaas)
        splitter(fixed_kaas)
        return node,time,temp,hum,pres
    else:
        print(json_string)



def splitter(event_line):

 #   print(event_line)
    # Split the line into key-value pairs
    pairs = event_line.split(',')

    # Initialize variables
    time = None
    node = None
    temp = None
    hum = None
    pres = None

    # Iterate through key-value pairs
    for pair in pairs:
        key, value = pair.split(':',1)
        key = key.strip()
        value = value.strip()
#        if value.lower() == 'null':
#            value = '0'
        value = ''.join(char for char in value if char.isdigit() or char in ('.', '-'))
        # Assign values based on the key
        if key == 'time':
            time = value if value else 0
        elif key == 'node':
            node = int(value) if value else 0
        elif key == 'temp':
            temp = float(value) if value else 0
        elif key == 'hum':
            hum = float(value) if value else 0
        elif key == 'pres':
            pres = float(value) if value else 0

   # return  time, node, temp, hum, pres

    print(f'Node: {node}')
    print(f'Time: {time}')
    print(f'Temperature: {temp}')
    print(f'Humidity: {hum}')
    print(f'Pressure: {pres}')
    insert_data(conn, node, time, temp, hum, pres)
    #return node,time,temp,hum,pres



















if __name__ == "__main__":
    # SQLite database file
    db_file = "WeatherData.db"

    # Connect to the database
    conn = create_connection(db_file)

    # Create the table if not exists
    create_table(conn)

    while True:
        ip_range = get_ip_range()

        if ip_range:
            print(f"Generated IP range: {ip_range}")
            scan_and_connect(ip_range)
        else:
            print("Failed to generate IP range.")
        print("begin opnieuw")
