/*
 * Web Server Module
 * 
 * Provides a web interface for displaying current sensor data.
 */

#include <WebServer.h>

WebServer server(80);

String getStatusPage() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Homelab Status</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        html {
            height: 100%;
        }
        body {
            font-family: Bahnschrift, DIN, sans-serif;
            font-size: 4vh;
            color: white;
            min-height: 100%;
            margin: 0;
            background: #25262b;
            display: grid;
            place-items: center;
            overflow: hidden;
        }
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            height: 100%;
        }
        .card {
            background: white;
            border-radius: 10px;
            aspect-ratio: 1;
            display: flex;
            flex-direction: column;
            background: rgba(255, 255, 255, 0.05);
        }
        .label {
            font-size: 1.4em;
            letter-spacing: 0.1em;
            padding: 17px 20px 13px;
            margin-bottom: 10px;
            border-bottom: solid 5px #25262b;
        }
        .row {
            padding: 5px 20px;
        }
        .value {
            font-size: 3em;
            font-weight: 300;
        }
        .unit {
            font-size: 1.4em;
            margin-left: 5px;
        }
        .invalid {
            color: orange;
        }
    </style>
</head>
<body>
    <div class="grid">)";

    // Generate sensor cards
    for (int i = 0; i < NUM_SENSORS; i++) {
        html += "<div class='card'>";
            html += "<div class='label'>" + String(sensors[i].label) + "</div>";
            html += "<div class='row" + String(sensors[i].isValid ? "" : " invalid") + "' " 
                  + "id='" + String(sensors[i].id) + "-row'>";
                html += "<span class='value' id='" + String(sensors[i].id) + "-value'>";
                    html += sensors[i].isValid ? String(int(sensors[i].value)) : "-";
                html += "</span>";
                html += "<span class='unit'>";
                    html += String(sensors[i].unit);
                html += "</span>";
            html += "</div>";
        html += "</div>";
    }

    html += R"(
    </div>

    <script>
        function refreshData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    data.forEach(sensor => {
                        const rowEl = document.getElementById(sensor.id + '-row');
                        const valueEl = document.getElementById(sensor.id + '-value');
                        if (sensor.isValid) {
                            valueEl.textContent = sensor.value.toFixed(0);
                            rowEl.classList.remove('invalid');
                        } else {
                            valueEl.textContent = '-';
                            rowEl.classList.add('invalid');
                        }
                    });
                });
        }
        refreshData();
        setInterval(refreshData, 10000);
    </script>
</body>
</html>
    )";

    return html;
}

/*
 * Request Handlers
 */

void handleRoot() {
    server.send(200, "text/html", getStatusPage());
}

void handleData() {
    // Generate JSON response with current sensor values
    String json = "[";
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"id\":\"" + String(sensors[i].id) + "\",";
        json += "\"value\":" + String(sensors[i].value) + ",";
        json += "\"isValid\":" + String(sensors[i].isValid ? "true" : "false");
        json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
}

/*
 * Server Management
 */

void setupServer() {
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
    Serial.println("HTTP server started");
}

void handleServer() {
    server.handleClient();
}
