# XT211 Meter for Home Assistant

This custom integration for Home Assistant allows you to monitor data from the **XT211** electricity meter via the **MQTT** protocol. The messages are sent by a module (which is not the subject of this description) located in the electricity meter that reads data from the RS485 DLMS/COSEM interface, converts them to MQTT messages, and sends them to the Home Assistant system.
In addition to creating sensors directly in Home Assistant, the integration can automatically forward the measured data to an **InfluxDB v2** database for long-term archiving and analysis.

## Key Features

* **Real-time Sensors**: Automatic creation of sensors for voltage, current, power, and frequency on individual phases.
* **Energy Dashboard Support**: Consumption sensors (OBIS codes 1.8.x and 2.8.x) are fully compatible with the Home Assistant Energy Dashboard (`total_increasing` state classes).
* **InfluxDB v2 Integration**: Direct data forwarding to InfluxDB using the Line Protocol with exact measurement timestamps directly from the device.
* **Diagnostic Data**: Monitoring of battery status, Wi-Fi signal strength (RSSI), number of wake-ups, and system status.
* **Easy Configuration**: Full support for initial setup and subsequent changes (Options Flow) directly in the user interface.

## Installation

1. Copy the `xt211` folder to the `custom_components` directory in your Home Assistant installation.
2. Restart Home Assistant.
3. In the menu **Settings -> Devices & Services**, click on **Add Integration** and search for "XT211 Meter".

## Configuration

During the setup, you provide the following:

### MQTT Settings
* **Data Topic**: Topic for measured values (default: `elektromer/data`).
* **Status Topic**: Topic for status messages (default: `elektromer/status`).

### InfluxDB v2 Settings (Optional)
* **Host & Port**: Server address (e.g., `192.168.1.50`, port `8086`).
* **Organization & Bucket**: Your data from the InfluxDB administration.
* **Token**: V2 API Token for writing data.

## Supported OBIS Codes

| Code | Sensor Name | Unit |
| :--- | :--- | :--- |
| **1.8.0** | Total Import Energy | kWh |
| **2.8.0** | Total Export Energy | kWh |
| **16.7.0**| Active Power (Total) | W |
| **32.7.0**| Voltage L1 | V |
| **31.7.0**| Current L1 | A |

## Technical Details

* **Version**: 0.2.6
* **Dependencies**: Requires a functioning MQTT broker.
* **Data Processing**: Data is sent to InfluxDB to the `home_energy` measurement with the `meter` tag.

### MQTT Data Structure

The integration processes JSON messages published on the following topics:

#### 1. Measured Values (Topic: elektromer/data)
This message carries the main data from the electricity meter. The integration automatically creates sensors for all OBIS codes found in the "values" section.

Message example:

    {
      "datetime": "Sat Feb 21 20:27:33 2026",
      "data": {
        "reading_datetime": "Sat 2026-02-21 20:27:09 GMT",
    	"first_boot":false,    
        "values": {
          "1.8.0": 7369.066,
          "1.8.1": 1338.005,
          "1.8.2": 6031.06,
          "1.8.3": 0,
          "1.8.4": 0,
          "96.1.1": "5100025085"
        }
      }
    }

#### 2. Device and Battery Status (Topic: elektromer/status)
The integration expects diagnostic messages on this topic. It distinguishes them by the main key in the JSON:

A) System Status (key "Status") - message example:

    {
      "datetime": "Sat Feb 21 20:27:35 2026",
      "Status": {
        "Status": "OK",
        "StatusText": "After wakeup",
        "Wakeups": 1072,
        "LastWait": 8,
        "FirstBootTime": "Tue 2026-02-10 12:07:07 GMT",
        "BuildDatetime":"2025-12-08 13:05:39",
        "Wifi": "-77"
      }
    }
Values StatusText, Wakeups, LastWait, FirstBootTime, and Wifi were added for debugging the external module, which is located at the electricity meter outside the house, and have no impact on functionality; they are merely displayed as sensors.

B) Battery Status (key "battery") - message example:

    {
      "datetime": "Sat Feb 21 20:27:38 2026",
      "battery": {
        "Voltage": 3.783,
        "SOC": 35.78
      }
    }

---
**Author**: [@psopso](https://github.com/psopso)