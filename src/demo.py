import requests
import json
import random
import time
import sys

BASE_URL = "http://127.0.0.1:8080"
HEADERS = {}

def print_step(title):
    print(f"\n{'='*60}")
    print(f"STEP: {title}")
    print(f"{'='*60}")
    time.sleep(1)

def print_json(data):
    print(json.dumps(data, indent=2))

def check_server():
    print_step("Checking Server Status")
    try:
        response = requests.get(f"{BASE_URL}/status")
        if response.status_code == 200:
            print(f"Server is ONLINE: {response.text}")
            return True
        else:
            print(f"Server returned error: {response.status_code}")
            return False
    except requests.exceptions.ConnectionError:
        print("ERROR: Could not connect to server.")
        print("Please ensure './build/et-server' is running in another terminal.")
        sys.exit(1)

def register_user(username):
    print_step(f"Registering User: {username}")
    response = requests.post(f"{BASE_URL}/register", params={"username": username})
    
    if response.status_code == 200:
        data = response.json()
        api_key = data["api_key"]
        print(f"Success! Received API Key: {api_key}")
        HEADERS["X-API-Key"] = api_key
    else:
        print("Registration failed.")
        sys.exit(1)

def simulate_traffic():
    print_step("Simulating System Traffic (Generating Logs)")
    
    functions = ["ImageProcess", "DataSync", "Encryption", "RenderFrame", "DB_Cleanup"]
    
    for i in range(1, 11):
        func = random.choice(functions)
        duration = random.randint(10, 500) 
        ram = random.randint(1024, 8192)   
        rom = random.randint(10, 100)
        msg = f"Routine execution of {func}"

        params = {
            "func": func,
            "msg": msg,
            "duration": duration,
            "ram": ram,
            "rom": rom
        }

        response = requests.post(f"{BASE_URL}/log", headers=HEADERS, params=params)
        print(f"[{i}/10] Logged {func}: RAM={ram}KB, Time={duration}ms -> Status: {response.status_code}")
        time.sleep(0.2)

def query_data():
    print_step("Querying All Data")
    response = requests.get(f"{BASE_URL}/query", headers=HEADERS)
    data = response.json()
    print(f"Retrieved {len(data)} total logs from database.")

def query_analytics():
    print_step("Analytics: High RAM Usage (> 6000 KB)")
    response = requests.get(f"{BASE_URL}/query/ram", headers=HEADERS, params={"min": 6000})
    data = response.json()
    print_json(data)

    print_step("Analytics: Long Duration (> 200 ms)")
    response = requests.get(f"{BASE_URL}/query/duration", headers=HEADERS, params={"min": 200})
    data = response.json()
    print_json(data)

def query_heavy_tasks():
    print_step("Advanced Analysis: 'Heavy' Tasks")
    print("Finding intersection of High RAM AND Long Duration...")
    
    response = requests.get(
        f"{BASE_URL}/query/heavy", 
        headers=HEADERS, 
        params={"min_ram": 5000, "min_dur": 300}
    )
    
    data = response.json()
    
    if len(data) == 0:
        print("No heavy tasks found based on current random data.")
    else:
        print(f"Found {len(data)} critical performance bottlenecks:")
        print_json(data)

def main():
    print("\n*** ExecTrace Demo Client ***\n")
    
    if check_server():
        register_user("demo_admin")
        simulate_traffic()
        query_data()
        query_analytics()
        query_heavy_tasks()
        
    print("\n*** Demo Complete ***")

if __name__ == "__main__":
    main()