# openUTD
Finding open study rooms at UTD with ESP8826 microchips. Contains arduino sketches I wrote at HackUTD 2019

Hackathon project. We wanted to find out if there were people in study rooms. The goal was to make a system that required no user interaction -- if they had to sign into the system, then there would be people who don't sign into the system.
We thought that most people who go to study here typically have a phone or laptop with them. After some research, I found that these internet-connected devices broadcast their MAC address every now and then.
Tracking those broadcasts provides a proxy for seeing how many devices/people are in the room.
The microchip keeps track of the number of devices in the room, and sends that to openutd.com which displays the count.
No MAC addresses are sent in the web request, as a safety precaution. I didn't do the web development, that was the other members in the hackathon group. I worked with the microchip to provide data for the website.

[openutd.com](openutd.com "Our Website!")

Captures a stream of mac addresses, and each hit of the last two character code adds a drop to a bucket labeled with that code
Over time, the buckets leak and lose their drops. This means that if there is an address that is just noise, the drop will go in and come out later.
If there is an address that is coming from a device, there is a stream that comes in at a greater speed than the slow leak, resulting in a non-empty bucket.
If there is an address from a router, then the bucket overflows. Buckets that have been overflowing for longer than an hour are added to a list of constant devices that exist in the vicinity but aren't a person


The wifi sniffing (esppl functions and struct) is from Ricardo Oliveira:
# Friend Detector by Ricardo Oliveira, forked by Skickar 9/30/2018

This project requires: A NodeMCU, a mini-breadboard (or full sized one), and a 4 pin RGB LED. 

 The function of this code is to read nearby Wi-Fi traffic in the form of packets. These packets are compared to a list of MAC addresses we wish to track, and if the MAC address of a packet matches one on the list, we turn on a colored LED that is linked to the user owning the device. 

 For example, when my roommate comes home, the	transmissions from his phone will be detected and cause the blue LED to turn on until his phone is no longer detected. It can detect more than one phone at a time, meaning if my phone (red) and my roommate's phone (blue) are both home, the LED will show purple. 

## For more information on the design go to: https://www.hackster.io/ricardooliveira/esp8266-friend-detector-12542e


