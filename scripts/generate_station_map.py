import csv
import datetime
from pathlib import Path

csv_file_path = 'stations.csv'
header_file_path = '../include/GeneratedStationMap.h'
cpp_file_path = '../src/GeneratedStationMap.cpp'

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

current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# Header: extern declaration only (single storage defined in .cpp)
header_content = f"""#ifndef STATION_MAP_H
#define STATION_MAP_H

/**
 * Auto-generated station map header file
 * Generated on: {current_time}
 *
 * IMPORTANT: Only declares the stationMap symbol.
 * The definition is generated in src/GeneratedStationMap.cpp to avoid
 * duplicate copies across translation units.
 */

#include <string>
#include <map>
#include "Station.h"

extern std::map<int, Station> stationMap;

#endif // STATION_MAP_H
"""

# CPP: single definition
cpp_content = f"""// Auto-generated on: {current_time}
#include "GeneratedStationMap.h"

std::map<int, Station> stationMap = {{
"""

for station in stations:
    stop_id = station["stop_id"].replace('"', '\\"')
    name = station["name"].replace('"', '\\"')
    cpp_content += f'    {{{station["ledIndex"]}, Station("{stop_id}", "{name}")}},\n'

cpp_content += "};\n"

# Write files
Path(header_file_path).write_text(header_content, encoding="utf-8")
Path(cpp_file_path).write_text(cpp_content, encoding="utf-8")

print(f"Generated {header_file_path} and {cpp_file_path} successfully.")