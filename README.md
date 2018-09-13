# X-10 Controller
X-10 Controller Interface for the FireCracker CM17a Module

## Description
This is designed to run on an ESP8266. I currently have it running on an AdaFruit Feather Huzzah with a 0.96" OLED Display. It starts by connecting to WiFi and then to the specified MQTT Server and awaits commands. You can test it out by running MQTT.Fx for the Mac or Windows or whatever your favorite client is. After launching, click the ear and make sure the IP Address of your MQTT server is in the Broker Address field and the Broker Port is 1883. Then connect to the server, use the SubScribe Topic you specified in the firmware file and post a message.

## MQTT Message Format
For the message, it should be in the following format in all capital letters:

`COMMAND-HOUSE-DEVICE`

COMMAND should be one of: ON, OFF, DIM, BRIGHT
HOUSE should be a single capital letter of A through P specifying the House Code of the device
DEVICE Should either be a number between 1 and 16, ALL to address all devices, or LAMPS to address Lamps only

Each data piece should be separated by a single hyphen.

So the Following examples should work:

`ON-D-12` to Turn On Device 12 in House D
`OFF-B-LAMPS` to Turn Off All Lamps in House B
`ON-A-ALL` to Turn On All Lamps and Appliances in House A

## OpenHab Setup
### Items
To set up an X10 Module in the items file, here is an example to get it to function in an on off capacity
```
Switch X10ApplianceModule1 "Appliance" <switch> {mqtt=">[broker:MySmartHome/utilities/X10Controller:command:ON:ON-A-1],>[broker:MySmartHome/utilities/X10Controller:command:OFF:OFF-A-1]"}
Dimmer X10LampModule1 "Lamp" <light> {mqtt=">[broker:MySmartHome/utilities/X10Controller:command:ON:ON-A-2],>[broker:MySmartHome/utilities/X10Controller:command:OFF:OFF-A-2]"}
```

### Sitemap
```
sitemap home label="My Home"
{
        Frame label="Room 1"
        {
                Switch item=X10ApplianceModule1
        }

        Frame label="Room 2"
        {
                Switch item=X10LampModule1
        }
}
```

### Rules
I found I didn't need to set any rules to use this, but I also haven't got the dimmer working yet

## Limitations
Although the FireCracker can transmit any house code, the codes are limited to the House Code of the Receiving Module. To use more than one house, you will need multiple receivers.

Even though the Dim and Bright commands require a Device ID, they will only function on the last device that had an On/Off command sent and in 20% increments from their current setting.

I don't believe there is currently a way to poll the status of devices with the FireCracker, but using OpenHab does a nice job of keeping track of the state of the devices unless you change them either through another X10 Device or turn the device on through turning the connected device off and on.
