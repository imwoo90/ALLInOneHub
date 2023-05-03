FILE(GLOB lib_sources
    TM1637/*.c*
    NeoPixel/Adafruit_NeoPixel.cpp
)

target_sources(app PRIVATE ${lib_sources})

target_include_directories(app PRIVATE
    ./
    TM1637
    NeoPixel
)