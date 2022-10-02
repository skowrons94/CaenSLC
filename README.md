# Requirements

The following ones can be installed from package manager, eg. on Ubuntu:

```bash
sudo apt install cmake libgsoap-dev libboost-all-devel libjsoncpp-dev
```

The CAEN drivers (in order of installation: CAENVMElib CAENComm CAENDigitizer) 
must be downloaded from their website and installed following their 
instructions. If the board is connect through the USB, also CAENUSBdrv is 
needed.

# Build / Install

To build the repository:

```bash
git clone https://baltig.infn.it/LUNA_DAQ/CaenSLC.git
cd CaenSLC && mkdir build && cd build/
cmake ..
make && sudo make install
```

After the build, two differents executables will be installed in 
```/opt/CaenSLC/```, one for the server (```CAENServer```), one for the client 
(```CAENClient```). You can add the ```export PATH=/opt/CaenSLC/${PATH}``` line 
to your ```bashrc``` file to run it from terminal. In addition, a folder called
"scripts" will be created in the installation directory where some examples of 
usage can be found.

For more informations, run ```CAENServer -h``` or ```CAENClient -h``` for help.
