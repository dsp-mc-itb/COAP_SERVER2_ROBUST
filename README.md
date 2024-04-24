# COAP_SERVER_ROBUST

## Installation Raspi camera in Raspi using C++ language

1. sudo raspi-config >> enable Camera
2. sudo apt-get update
3. sudo apt-get install libraspberrypi-dev libopencv-dev
4. Download [raspicam-0.1.9](https://sourceforge.net/projects/raspicam/files/raspicam-0.1.9.zip/download) library into your Pi.
5. install the library
```
cd raspicam-0.1.9 (go to the library folder)
mkdir build
cd build
cmake ..
make
sudo make install
```
6. Finally, create,compile and execute the sample program

Full documentation [Raspi-Camera](https://sourceforge.net/projects/raspicam/files/README/download)

## Install Libcoap
```
sudo chmod 777 autogen.sh
./autogen.sh
./configure --disable-documentation --with-openssl
make
sudo make install
```
## Install cmake
sudo apt install cmake

## Edit and setting connection Raspi OS 12 

[connection]
Cara Setting Wifi - OS Raspi 12 Bookworm terbaru

1. sudo nmcli c add type wifi ifname wlan0 con-name "se" ssid my_ssid
2. Kemudian update /etc/NetworkManager/system-connections

Seperti sample dibawah ini ...
```
id=TP-Link_AC44
uuid=2f60f269-6d6c-42ac-a903-beba961d1c63
type=wifi
interface-name=wlan0

[wifi]
mode=infrastructure
ssid=TP-Link_AC44

[wifi-security]
auth-alg=open
key-mgmt=wpa-psk
psk=aaaaaaaa

[ipv4]
method=auto

[ipv6]
addr-gen-mode=default
method=auto

[proxy]
```
