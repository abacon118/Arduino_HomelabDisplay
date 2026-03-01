# HTTP API monitoring on port 2002
# Written by Copilot

from flask import Flask, request, jsonify
import psutil, os

app = Flask(__name__)
flask_api_key = os.getenv("flask_api_key", "defaultkey")

def get_temp():
    with open("/sys/class/thermal/thermal_zone0/temp") as f:
        return int(f.read()) / 1000

@app.route("/metrics")
def metrics():
    # Check API key in query string or header
    key = request.args.get("key") or request.headers.get("X-API-Key")
    if key != flask_api_key:
        return jsonify({"error": "Unauthorized"}), 401

    return jsonify({
        "cpu": psutil.cpu_percent(),
        "memory": psutil.virtual_memory().percent,
        "temp": get_temp()
    })

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=2002)
