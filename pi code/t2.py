#!/usr/bin/env python3
import subprocess
import threading
import json
import os
import ipaddress
import signal
# import shlex
import time

ssid = "CircuitCraft-WiFi"
password =""
def capture_output(process, stream, lines):
    for line in stream:
        lines.append(line.strip())


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
                painlessmesh_command = f"painlessMeshBoost -c {ip_address} >> boost_output.txt"
                process = subprocess.Popen(painlessmesh_command, shell=True)

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

if __name__ == "__main__":
    while True:
        ip_range = get_ip_range()

        if ip_range:
            print(f"Generated IP range: {ip_range}")
            scan_and_connect(ip_range)
        else:
            print("Failed to generate IP range.")
        print("begin opnieuw")
