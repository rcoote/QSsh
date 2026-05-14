# Background
 
 Inspired by "It's a UNIX system I know this" - one of my favorite childhood movies I created the "hostmanager" example, which i derived right away from the "sftpfsmodel" test/example.

<img width="1454" height="1400" alt="image" src="https://github.com/user-attachments/assets/f9b29b05-7b7b-4a04-97ae-3feba22a7d27" />
https://www.reddit.com/r/MovieDetails/comments/89p4n4/in_jurassic_park_the_infamous_its_a_unix_system_i/?show=original

# Building (on Ubuntu 24.04.3 LTS)

# Requirements

## QT
Find QT Online Installer from QT Downloads Archive:
* Download "qt-online-installer-linux-x64-4.10.0.run" and run
* Install Standard Desktop Development QT6 to /home/<USER>

## Botan
sudo apt install libgl1-mesa-dev
sudo apt-get install pkg-config
sudo apt-get install libbotan-2-dev

# MQTT
## Build MQTT for QT under Windows
Follow these instructions:
https://www.emqx.com/en/blog/how-to-create-an-mqtt-application-in-qt6#common-compiling-errors-in-qt-creator

## Linux
Under Linux use your repo manager zypper, apt-get to install libqt6mqtt.

# Installation

Todo list:
*  Add 2D View
*  Add 3D View
*  Add MQTT Client (each host can supply an MQTT Data to a Broker)
*  Add terminal to connect remotley to any ssh host

