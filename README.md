# COAP_CLENT_ROBUST

# Installation Raspi camera in Raspi using C++ language

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