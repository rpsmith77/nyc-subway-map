import csv
import datetime

# Define the path to the CSV file and the output header file
csv_file_path = 'stations.csv'
header_file_path = '../include/station_map.h'

# Read the CSV file and parse the data
stations = []
led_index = 0
with open(csv_file_path, mode='r') as csv_file:
    csv_reader = csv.DictReader(csv_file)
    for row in csv_reader:
        stations.append({
            'stop_id': row['stop_id'],
            'name': row['name'],
            'ledIndex': led_index
        })
        led_index += 1

# Get current timestamp
current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# Generate the station_map.h file content
header_content = f"""#ifndef STATION_MAP_H
#define STATION_MAP_H

/**
 * Auto-generated station map header file
 * Generated on: {current_time}
 * 
 * IMPORTANT: LED indices in this map correspond directly to the physical
 * order of LEDs as they are soldered in the hardware. The order of stations
 * in stations.csv determines these indices.
 */

#include <string>
#include <map>
#include "MtaHelper.h"

static std::map<int, Station> stationMap = {{
"""

for station in stations:
    header_content += f'    {{{station["ledIndex"]}, Station("{station["stop_id"]}", "{station["name"]}")}},\n'

header_content += """};

#endif // STATION_MAP_H
"""

# Write the generated content to the station_map.h file
with open(header_file_path, mode='w') as header_file:
    header_file.write(header_content)

print(f"Generated {header_file_path} successfully.")