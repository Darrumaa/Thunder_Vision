import requests

# ESP8266 IP Address (replace with actual IP)
ESP_IP = "http://ESP_IP_ADDRESS"

# API Key (must match the one saved in ESP8266)
API_KEY = "YOUR_API_KEY"

# Function to control the relay
def control_relay(room, state):
    url = f"{ESP_IP}/relay?apikey={API_KEY}&room={room}&state={state}"
    
    try:
        response = requests.get(url, timeout=5)  # 5-second timeout
        if response.status_code == 200:
            print(f"Success: {response.text}")
        else:
            print(f"Error {response.status_code}: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Request failed: {e}")

# Example: Turn ON relay for room "VIB109"
control_relay("VIB109", "ON")

# Example: Turn OFF relay for room "VIB109"
control_relay("VIB109", "OFF")
