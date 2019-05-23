# MDP_16
This repository contains the codes and libraries used for every transmitting method in testing the capabilities of LoRaWAN, using a single channel (868.1 or 868.3 MHz) and a single spreading factor that can be changed from the "dr" variable in the different arduino scripts.
First, download the lmic-master (http://wiki.lahoud.fr/doku.php?id=exploring_lorawan) and the RadioHead libraries.
For every method of transmission, the sender should use the proper LMIC library in the Arduino app: right click on the Arduino icon and enter to "Show Package Content" on mac, or "Open File Location" on Windows.
Then enter to Java, libraries and then to arduino-lmic-master on mac, or enter directly to libraries on windows and then to arduino-lmic-master.
Now open src file, and replace the lmic folder with the one unique for every method.
Open the joint arduino code (.ino), and you're good to go!
