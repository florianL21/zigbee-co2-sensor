# ESP32-H2/C6 CO2 Zigbee Device

This project is a Zigbee device based on the ESP32-H2 / ESP32-C6 that can read CO2 sensor values from a sensirion SCD-4X based sensor via i2c.

Power saving features are not implemented. So this is not suitable for operating on battery power at this point.

## Features

- [x] SCD-4X temperature sensor
- [x] SCD-4X humidity sensor
- [x] SCD-4X CO2 sensor

Exposed values (example screenshot from Home Assistant ZHA):

![Home Assistant UI](images/ha.png)

## Hardware

Any variants/breakout boards that use the following components can be used:

- [ESP32-H2](https://www.espressif.com/en/products/socs/h2/overview)
- [ESP32-C6](https://www.espressif.com/en/products/socs/c6/overview)
- [Sensirion SCD-41](https://sensirion.com/products/catalog/SCD41)

But this project specifically uses the following breakout boards for convenience:
- [Seeed Sudio XIAO-ESP32C6](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C6-p-5884.html)
- [Adafruit SCD-41](https://www.adafruit.com/product/5190)


## GPIO

The firmware is configured to use the following PIO pins by default:

| GPIO   | Function              |
| ------ | --------------------- |
| GPIO22 | I2C SDA to SCD-41     |
| GPIO23 | I2C SCL to SCD-41     |

With the suggested breakout boards from above it would be wired up like this:

![Screenshot of erase flash option in the web UI](images/wire_diagram.png)

## Flashing

You can very easily flash the firmware using a handy web interface.
Simply go to the link below and click the connect button (note that you will have to use a supported browser for this to work):

[Web flash tool](https://florianl21.github.io/zigbee-co2-sensor/)

## Un-pairing/factory reset

The firmware does currently not have any mechanic to support factory resetting the device from a user perspective.
If the device it removed from the zigbee coordinator while the device is connected it is automatically un-paired and will go into a state where it is ready to be re-paired.
If for some reason this is not an option, you can just erase the devices entire flash effectively "factory resetting" it. This can also be done via the web interface by selecting the `Erase device` checkbox when flashing the firmware:

![Screenshot of erase flash option in the web UI](images/factory_reset.png)

# Credits

Thanks to @xmow49 for his [zigbee demo sensor repository](https://github.com/xmow49/ESP32H2-Zigbee-Demo).
The code in this repository is based on his implementation with some significant modifications
