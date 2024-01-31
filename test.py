import subprocess
import json
import os
import ipaddress

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
    best_strength = -100  # Set an initial value
    best_ip = None

    # Step 1: Scan for ESP32 devices using nmap
    nmap_command = f"nmap -p 5555 --open {ip_range}"
    result = subprocess.run(nmap_command, shell=True, capture_output=True, text=True)
    output_lines = result.stdout.split('\n')

    for line in output_lines:
        if "Nmap scan report for" in line:
            # Extract IP address from the nmap output
            ip_address = line.split()[-1]

            # Step 2: Connect to the ESP32 using painlessmeshboost
            painlessmesh_command = f"painlessmeshboost -c {ip_address}"
            mesh_result = subprocess.run(painlessmesh_command, shell=True, capture_output=True, text=True)
            mesh_output = mesh_result.stdout.strip()

            # Step 3: Parse the received message and extract WiFi strength
            try:
                message_json = json.loads(mesh_output)
                wifi_strength = message_json.get("wifi_strength", None)

                # Save WiFi strength to a file
                if wifi_strength is not None:
                    with open(f"{ip_address}_wifi_strength.txt", "w") as file:
                        file.write(str(wifi_strength))

                    # Step 4: Determine the best connection based on WiFi strength
                    if wifi_strength > best_strength:
                        best_strength = wifi_strength
                        best_ip = ip_address

            except json.JSONDecodeError:
                print(f"Error decoding JSON for {ip_address}")

    # Step 5: Re-establish a connection with the ESP32 having the best WiFi strength
    if best_ip is not None:
        print(f"Best WiFi strength found at {best_ip}")
        # Add the necessary command to re-establish a connection

if __name__ == "__main__":
    # Get the IP range dynamically
    ip_range = get_ip_range()

    if ip_range:
        print(f"Generated IP range: {ip_range}")

        # Scan and connect based on the generated IP range
        scan_and_connect(ip_range)
    else:
        print("Failed to generate IP range.")
