FILE(GLOB_RECURSE app_sources
    main.cpp
    app/superfect_protocol/*.c*
    app/system/*.c*
    app/neopixel/superfect_neopixel.cpp
    app/ps2_keyboard/ps2_keyboard.cpp
    # app/model/*.c*
    # app/view/*.c*
    # device/Relay/*.c*
    # device/Keypad_4x3/*.c*
    # device/BanknoteReader/*.c*
)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE
    ./
    app/superfect_protocol/
    app/system/
    # app/model
    # app/view
)