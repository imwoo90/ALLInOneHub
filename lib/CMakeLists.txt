FILE(GLOB lib_sources
    TM1637/*.c*
    NeoPixel/Adafruit_NeoPixel.cpp
    # BusIo/Adafruit_I2CDevice.cpp
    # RTClib/src/RTClib.cpp
    # RTClib/src/RTC_DS3231.cpp
    # arduino_adaptor/WString.cpp
    # arduino_adaptor/dtostrf.c
    # arduino_adaptor/itoa.c
)

target_sources(app PRIVATE ${lib_sources})

target_include_directories(app PRIVATE
    arduino_adaptor
    TM1637
    NeoPixel
    # BusIo
    # RTClib/src
)