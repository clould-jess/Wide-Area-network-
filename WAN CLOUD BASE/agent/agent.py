import time
import psutil
import requests
from datetime import datetime, UTC

API_URL = "http://localhost:8000/metrics"  # if running on same PC
SERVER_ID = "srv-001"

def collect():
    cpu = psutil.cpu_percent(interval=1)
    ram = psutil.virtual_memory().percent
    disk = psutil.disk_usage("C:\\").percent  # Windows drive
    uptime = int(time.time() - psutil.boot_time())
    return {
        "server_id": SERVER_ID,
        "timestamp": datetime.now(UTC).isoformat(),
        "cpu_percent": cpu,
        "ram_percent": ram,
        "disk_percent": disk,
        "uptime_seconds": uptime,
    }

def main():
    while True:
        payload = collect()
        try:
            r = requests.post(API_URL, json=payload, timeout=8)
            print("sent", r.status_code, payload)
        except Exception as e:
            print("send failed:", e)
        time.sleep(30)

if __name__ == "__main__":
    main()
