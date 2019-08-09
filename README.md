# SdrRtlTuner_Windows_Linux_Macx
 SdrRtlTuner for Windows Linux Macx
 
 Sdr library took from https://github.com/hmatuschek/libsdr
 
 A simple software defined radio (SDR) application.
 GNU radio not used because of windows compile problems.
 
 Used libraries are:
 sudo apt install libfftw3-dev 
 sudo apt install libportaudio2 
 sudo apt install portaudio19-dev
 
 Install Rtl-sdr Tools
 
 git clone git://git.osmocom.org/rtl-sdr.git
 cd rtl-sdr/
 mkdir build
 cd build
 cmake ../
 make
 sudo make install
 sudo ldconfig
