
# NYC Subway Map LED Visualization

A real-time visualization of NYC subway train locations using an ESP32 microcontroller and LED strips. This project creates a physical illuminated map that shows trains arriving at stations throughout the New York City subway system by lighting up LEDs at station locations when trains are present.

## How It Works

The system connects to the MTA's real-time data feeds through a custom MTAPI server. As trains arrive at stations across the NYC subway system, corresponding LEDs on your physical map light up in the color of the train line (e.g., yellow for the N/Q/R lines, red for the 1/2/3, etc.). The result is a living, breathing visualization of the entire subway network.

## Hardware Requirements

-   ESP32 development board
-   WS2812B LED strips (minimum 500 LEDs for full subway system)
-   5V power supply (capacity depends on your LED count; typically 10A minimum)
-   Wire for connections
-   Physical subway map for mounting LEDs
-   Mounting materials
-   Optional: 2 additional LEDs for error status indicators

## Software Dependencies

-   PlatformIO  (recommended) or Arduino IDE
-   Libraries:
    -   FastLED
    -   ArduinoJSON
    -   WiFi
    -   HTTPClient
    -   Time

## Project Structure

    ├── include/
    │ ├── MtaHelper.h # Helper functions for MTA data handling
    │ ├── station_map.h # Maps LED indices to station IDs
    │ └── WifiCredentials.h # WiFi connection credentials
    ├── src/
    │ └── main.cpp # Main application code
    ├── MTAPI/ # Server component that interfaces with MTA API
    └── platformio.ini # PlatformIO configuration

### Key Components

-   **main.cpp**: Core logic for fetching data, managing LEDs, and handling errors
-   **MtaHelper.h**: Contains the  Station  and  Train  classes and helpers for API access
-   **station_map.h**: Maps LED indices to MTA station IDs (auto-generated)
-   **WifiCredentials.h**: Contains your WiFi credentials and server settings
-   **[MTAPI Server](https://github.com/rpsmith77/MTAPI)**: Python-based service that interfaces with the official MTA API

## Setup Instructions

### 1. Hardware Setup

1.  Connect the WS2812B LED data line to ESP32 pin 10 (D7) for subway LEDs
2.  Connect optional error indicator LEDs data line to pin 5 (D2)
3.  Connect ESP32 and LED strips to 5V power supply
4.  Mount LEDs on your physical map, positioning each LED at its corresponding station

### 2. WiFi Configuration

Create a  `WifiCredentials.h`  file in the  include  directory:
```c++
#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H
// WiFi credentials
const  char*  WIFI_SSID  =  "your_wifi_ssid";
const  char*  WIFI_PASSWORD  =  "your_wifi_password";
// API server details
const  std::string  SERVER_HOST  =  "your_mtapi_server_ip";
const  std::string  SERVER_PORT  =  "5000";
const  std::string  API_BY_ID  =  "http://"  +  SERVER_HOST  +  ":"  +  SERVER_PORT  +  "/by-id/";
#endif // WIFI_CREDENTIALS_H
```
### 3. MTAPI Server Setup

1.  Clone the MTAPI repository (included in this project or available at  GitHub)
    
2.  (Depricated) Get an MTA API key from  MTA Developer Portal
    
3.  Create a  `settings.cfg`  file based on  `settings.cfg.sample`:
```docker
	   MTA_KEY = 'your_mta_api_key' #deprecated
       STATIONS_FILE = 'data/stations.json'
       CACHE_SECONDS = 60
       MAX_TRAINS = 10
       MAX_MINUTES = 30
       THREADED = True
```
    
4.  Install Python dependencies:
```bash
    cd  MTAPI
    python3  -m  venv  .venv
    source  .venv/bin/activate
    pip  install  -r  requirements.txt
```
    
    
5.  Run the server:
    
    `python  app.py`
    
    Alternatively, use Docker:
    
    `docker-compose  up`
    

### 4. ESP32 Firmware Installation

Using PlatformIO:

1.  Open the project in PlatformIO
2.  Build the project
3.  Upload to your ESP32

Using Arduino IDE:

1.  Install all required libraries via the Library Manager
2.  Open  main.cpp  as an Arduino sketch
3.  Select ESP32 board in Board Manager
4.  Upload to your ESP32

## Station Mapping

The  station_map.h  file maps LED indices to MTA station IDs. Each LED index corresponds to a physical LED position on your map.

If you need to modify station positions **use the station map generation file in scripts**, or alternatively:

1.  Edit the  station_map.h  file to match your physical LED layout
2.  Make sure the LED indices match the order in which LEDs are connected in your physical installation

## How the Code Works

### Batch HTTP Requests

To avoid overwhelming the ESP32's memory, the code divides station data fetching into batches:

const  int  num_groups  =  45;

int  group_size  =  stationMap.size()  /  num_groups;

Each group of stations is fetched on a schedule to ensure all stations get updated within a minute.

### Memory Management

The code monitors heap memory usage and will reboot if memory gets too low:
``` c++
if  (free_heap  <  1.25  *  json_doc_size)  {
	ESP.restart();
}
```
### Error Handling

-   White LED: WiFi connected successfully
-   Blinking red LED: WiFi connection issue
-   The system tracks HTTP errors and will reboot if too many occur in sequence

## Troubleshooting

### LED Issues

-   **No LEDs light up**: Check power supply and connections
-   **Random flickering**: Ensure your power supply can handle the current draw
-   **Wrong stations light up**: Verify  station_map.h  matches your physical layout

### Connectivity Issues

-   **Blinking red LED**: WiFi connection failed. Check credentials in WifiCredentials.h
-   **Connected but no data**: Verify the MTAPI server is running and accessible
-   **Frequent reboots**: MTAPI server may be unreachable or returning errors

### MTAPI Server Issues

-   **API errors**: Check your MTA API key is valid
-   **Invalid station data**: Ensure your stations.json file is properly formatted
-   **Server crashes**: Check logs for Python errors, may need more memory

## Advanced Configuration

-   Adjust LED brightness in  setup()  using  FastLED.setBrightness()
-   Modify  fetch_interval  to change how often station data updates
-   Change  json_doc_size  if you encounter JSON parsing errors
-   Enable  DEBUG  mode for additional diagnostics

## Credits

This project uses:

-   MTAPI  for interfacing with MTA data
-   FastLED  for LED control
-   ArduinoJson  for JSON parsing

## License

MIT License

----------

_This project is not affiliated with or endorsed by the Metropolitan Transportation Authority (MTA)._
