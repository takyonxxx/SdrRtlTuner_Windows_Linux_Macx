# SdrRtlTuner_Windows_Linux_Macx
 <b>SdrRtlTuner for Windows Linux Macx</b>
 
 Sdr library was taken from here https://github.com/hmatuschek/libsdr
 
 A simple software defined radio (SDR) application.
 GNU radio not used because of windows compile problems.
 Windows libraries located in project folder libs.</br>
 For windows : Project should compile with MinGw64 in windows10.</br>
 
 <b>Used libraries are:</b></br>
 sudo apt install libusb-1.0-0-dev</br>
 sudo apt install libfftw3-dev </br>
 sudo apt install libportaudio2 </br>
 sudo apt install portaudio19-dev</br>
 
 <b>Install Rtl-sdr Tools</b></br>
 sudo apt  install cmake</br>
 sudo apt  install git</br>

 git clone git://git.osmocom.org/rtl-sdr.git</br>
 cd rtl-sdr/</br>
 mkdir build</br>
 cd build</br>
 cmake ../</br>
 make</br>
 sudo make install</br>
 sudo ldconfig</br>

<p align="center"><a href="https://github.com/takyonxxx/SdrRtlTuner_Windows_Linux_Macx/blob/master/rtl-sdr.jpg">
		<img src="https://github.com/takyonxxx/SdrRtlTuner_Windows_Linux_Macx/blob/master/rtl-sdr.jpg" 
		name="Image3" align="bottom" width="800" height="520" border="1"></a></p>
