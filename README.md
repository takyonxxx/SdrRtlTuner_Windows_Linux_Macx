# SdrRtlTuner_Windows_Linux_Macx
 <b>SdrRtlTuner for Windows Linux Macx</b>
 
 Sdr library was taken from here https://github.com/hmatuschek/libsdr
 
 A simple software defined radio (SDR) application.
 GNU radio not used because of windows compile problems.</br>
 
 <b>For windows :</b>
 Project should compile with MinGw64 in windows10.</br>
 Windows libraries located in project folder libs.</br>
 
 <b>For linux: </b></br>
 <b>Used libraries are:</b></br>
 sudo apt install libusb-1.0-0-dev</br>
 sudo apt install libfftw3-dev </br>
 sudo apt install libportaudio2 </br>
 sudo apt install portaudio19-dev</br>
 sudo apt install rtl-sdr
 sudo apt-get install librtlsdr-dev
  	
 <b>For macx: </b></br>
 <b>Used libraries are:</b></br>
 brew install libusb</br>
 brew install fftw</br>
 brew install rtl-sdr</br>
 brew install portaudio</br>
 
 
<b>Qt5</b> (http://qt-project.org) - Enables the libsdr-gui library implementing some graphical user interface elements like a spectrum view.</br>
<b>fftw3</b> (http://www.fftw.org) - Also required by the GUI library and allows for FFT-convolution filters.</br>
<b>PortAudio</b> (http://www.portaudio.com) - Allows for sound-card input and output.</br>
<b>librtlsdr</b> (http://rtlsdr.org) - Allows to interface RTL2382U based USB dongles.</br>

<p align="center"><a href="https://github.com/takyonxxx/SdrRtlTuner_Windows_Linux_Macx/blob/master/rtl-sdr.jpg">
		<img src="https://github.com/takyonxxx/SdrRtlTuner_Windows_Linux_Macx/blob/master/rtl-sdr.jpg" 
		name="Image3" align="bottom" width="800" height="520" border="1"></a></p>
