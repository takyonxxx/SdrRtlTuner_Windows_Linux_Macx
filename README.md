# SdrRtlTuner_Windows_Linux_Macx
 SdrRtlTuner for Windows Linux Macx
 
 Sdr library took from https://github.com/hmatuschek/libsdr
 
 A simple software defined radio (SDR) application.
 GNU radio not used because of windows compile problems. Windows libraries located in project folder libs.</br>
 Project should compile with MinGw64 in windows10.</br>
 
 Used libraries are:</br>
 sudo apt install libfftw3-dev </br>
 sudo apt install libportaudio2 </br>
 sudo apt install portaudio19-dev</br>
 
 Install Rtl-sdr Tools</br>
 
 git clone git://git.osmocom.org/rtl-sdr.git</br>
 cd rtl-sdr/</br>
 mkdir build</br>
 cd build</br>
 cmake ../</br>
 make</br>
 sudo make install</br>
 sudo ldconfig</br>
