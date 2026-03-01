#!/bin/bash
# setup_metrics_service.sh
# Bash script to install Flask metrics API as a systemd service
#replace: APP_PATH,WORK_DIR,USER,Environment=flask_api_key
#written with Microsoft Copilot
pip install flask psutil --break-system-packages

SERVICE_NAME="FlaskAPI-Metrics"
APP_PATH="/home/pi/scripts/FlaskAPI-Metrics.py"
WORK_DIR="/home/pi"
USER="pi"

# Create systemd service file
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"

echo "Creating systemd service at $SERVICE_FILE..."

sudo bash -c "cat > $SERVICE_FILE" <<EOL
[Unit]
Description=Flask Metrics API Service
After=network.target

[Service]
ExecStart=/usr/bin/python3 ${APP_PATH}
WorkingDirectory=${WORK_DIR}
Restart=always
User=${USER}
Environment=flask_api_key=randomstring

[Install]
WantedBy=multi-user.target
EOL

# Reload systemd to recognize new service
echo "Reloading systemd..."
sudo systemctl daemon-reload

# Enable service to start on boot
echo "Enabling service..."
sudo systemctl enable ${SERVICE_NAME}.service

# Start service immediately
echo "Starting service..."
sudo systemctl start ${SERVICE_NAME}.service

echo "Service ${SERVICE_NAME} installed and started!"
