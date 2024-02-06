#!/usr/bin/env python3
import subprocess
import json
import os
import ipaddress
import time

def get_ip_range():
    try:
        # Run ifconfig to get network interface information
        result = subprocess.run(["ifconfig", "wlan0"], capture_output=True, text=True)
        output = result.stdout

        # Extract the inet line containing the current IP and netmask
        inet_line = [line for line in output.split("\n") if "inet" in line][0]

        # Extract the IP and netmask from the inet line
        current_ip = inet_line.split(" ")[1]
        netmask = inet_line.split(" ")[3]

        # Calculate the network address based on the current IP and netmask
        network = ipaddress.IPv4Network(f"{current_ip}/{netmask}", strict=False)
        
        # Generate the /24 IP range
        ip_range = f"{network.network_address}/24"

        return ip_range
    except Exception as e:
        print(f"Error: {e}")
        return None

def scan_and_connect(ip_range):
    best_strength = -100  # Initialize with a low value
    best_ip = None

    for ip_address in ip_range:
        # Step 1: Check if the ESP32 is reachable
        ping_result = subprocess.run(["ping", "-c", "1", ip_address], capture_output=True, text=True)
        if "1 packets transmitted, 1 received" in ping_result.stdout:

            # Step 2: Connect to the ESP32 using painlessmeshboost
            painlessmesh_command = f"painlessmeshboost -c {ip_address} >> boost_output.txt"
            process = subprocess.Popen(painlessmesh_command, shell=True)

            time.sleep(1)  # Give the process some time to start

            # Step 3: Monitor the file size
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
                    process.send_signal(signal.SIGINT)  # Send an interrupt signal to the process
                    break

                last_size = current_size
                time.sleep(5)  # Wait for 5 seconds

            # Step 4: Parse the received message and extract WiFi strength
            with open("boost_output.txt", "r") as file:
                mesh_output = file.read().strip()

            try:
                message_json = json.loads(mesh_output)
                wifi_strength = message_json.get("wifi_strength", None)

                # Save WiFi strength to a file
                if wifi_strength is not None:
                    with open(f"{ip_address}_wifi_strength.txt", "w") as file:
                        file.write(str(wifi_strength))

                    # Step 5: Determine the best connection based on WiFi strength
                    if wifi_strength > best_strength:
                        best_strength = wifi_strength
                        best_ip = ip_address

            except json.JSONDecodeError:
                print(f"Error decoding JSON for {ip_address}")

    # Step 6: Re-establish a connection with the ESP32 having the best WiFi strength
    if best_ip is not None:
        print(f"Best WiFi strength found at {best_ip}")
        # Add the necessary command to re-establish a connection

if __name__ == "__main__":
    # Get the IP range dynamically
    ip_range = get_ip_range()

    if ip_range:
        print(f"Generated IP range: {ip_range}")
        scan_and_connect(ip_range)