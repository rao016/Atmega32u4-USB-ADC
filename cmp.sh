make
avrdude -p atmega32u4 -c usbasp-clone -U flash:w:TestAndMeasurement.hex