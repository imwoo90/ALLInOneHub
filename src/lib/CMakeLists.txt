FILE(GLOB lib_sources
    TM1637/*.c*
)

target_sources(app PRIVATE ${lib_sources})

target_include_directories(app PRIVATE
    ./
    TM1637
)