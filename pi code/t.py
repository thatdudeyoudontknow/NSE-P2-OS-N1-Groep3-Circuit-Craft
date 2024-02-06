#!/usr/bin/env python3
import subprocess
import threading
import json
# import os
import ipaddress
# import shlex
import time

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

# def scan_and_connect(ip_range):
#     best_strength = -100
#     best_ip = None

#     nmap_command = f"nmap -p 5555 --open {ip_range}"
#     result = subprocess.run(nmap_command, shell=True, capture_output=True, text=True)
#     output_lines = result.stdout.split('\n')

#     for line in output_lines:
#         if "Nmap scan report for" in line:
#             ip_address = line.split()[-1]
#             print("Found host at", ip_address)

#             painlessmesh_command = f"painlessMeshBoost -c {ip_address}"
#             print(painlessmesh_command)

#             try:
#                 process = subprocess.Popen(
#                     shlex.split(painlessmesh_command),
#                     stdout=subprocess.PIPE,
#                     stderr=subprocess.PIPE,
#                     text=True
#                 )

#                 stdout_lines = []
#                 stderr_lines = []

#                 stdout_thread = threading.Thread(target=capture_output, args=(process, process.stdout, stdout_lines))
#                 stderr_thread = threading.Thread(target=capture_output, args=(process, process.stderr, stderr_lines))

#                 stdout_thread.start()
#                 stderr_thread.start()

#                 process.wait()

#                 stdout_thread.join()
#                 stderr_thread.join()

#                 if process.returncode == 0:
#                     print("Mesh command completed successfully")
#                     mesh_output = '\n'.join(stdout_lines).strip()
#                     print("Mesh output:")
#                     print(mesh_output)

#                     try:
#                         message_json = json.loads(mesh_output)
#                         wifi_strength = message_json.get("wifi_strength", None)
#                         temperature = message_json.get("temp", None)
#                         humidity = message_json.get("hum", None)
#                         pressure = message_json.get("pres", None)

#                         print(f"WiFi Strength: {wifi_strength}")
#                         print(f"Temperature: {temperature}")
#                         print(f"Humidity: {humidity}")
#                         print(f"Pressure: {pressure}")

#                         if wifi_strength is not None:
#                             with open(f"{ip_address}_wifi_strength.txt", "w") as file:
#                                 file.write(str(wifi_strength))

#                             if wifi_strength > best_strength:
#                                 best_strength = wifi_strength
#                                 best_ip = ip_address

#                     except json.JSONDecodeError:
#                         print(f"Error decoding JSON for {ip_address}")
#                 else:
#                     print("Mesh command failed with return code:", process.returncode)

#             except Exception as e:
#                 print("Error running subprocess:", e)

#     if best_ip is not None:
#         print(f"Best WiFi strength found at {best_ip}")
#         # Add the necessary command to re-establish a connection

def scan_and_connect(ip_range):
    while True:
        process = subprocess.Popen(
            ["painlessmesh", "list", "--json", ip_range],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        stdout_lines = []
        stderr_lines = []

        stdout_thread = threading.Thread(target=capture_output, args=(process, process.stdout, stdout_lines))
        stderr_thread = threading.Thread(target=capture_output, args=(process, process.stderr, stderr_lines))

        stdout_thread.start()
        stderr_thread.start()

        process.wait()

        stdout_thread.join()
        stderr_thread.join()

        if process.returncode == 0:
            print("Mesh command completed successfully")
            mesh_output = '\n'.join(stdout_lines).strip()
            print("Mesh output:")
            print(mesh_output)

            try:
                message_json = json.loads(mesh_output)
                wifi_strength = message_json.get("wifi_strength", None)
                print(f"WiFi Strength: {wifi_strength}")
                some_threshold = -60
                # Add a condition to break the loop if necessary
                if wifi_strength is None or wifi_strength < some_threshold:
                    break

            except json.JSONDecodeError:
                print("Failed to decode JSON")

        # Add a delay to avoid overloading the system
        time.sleep(5)


if __name__ == "__main__":
    ip_range = get_ip_range()

    if ip_range:
        print(f"Generated IP range: {ip_range}")
        scan_and_connect(ip_range)
    else:
        print("Failed to generate IP range.")
