## Flask API

CPU Usage, Memory and Temperature of a Raspberry Pi or Orange Pi can be monitored by running a small Python script as a service.  This service uses the Flask API library.

### Installation steps:
1. Ensure Python3 is installed
2. Download FlaskAPI_Setup.sh and FlaskAPI-Metrics.py
3. In FlaskAPI_Setup.sh edit the following:
  - APP_PATH
  - WORK_DIR
  - USER
  - Environment=flask_api_key=
      The flask_api_key can be a string of random text.  This must match secrets.h
 4. Make FlaskAPI_Setup.h executable
      chmod +x FlaskAPI_Setup.sh
 5. Run FlaskAPI_Setup.h
      ./FlaskAPI_Setup.h
