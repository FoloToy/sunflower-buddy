idf_component_register(
    SRCS "src/bsp_i2c.cpp"
         "src/bsp_button.cpp"
         "src/bsp_led.cpp"
         "src/bsp_audio.cpp"
         "src/bsp_power.cpp"
         "src/bsp_motor.cpp"
         "src/bsp_wakeup.cpp"
         "src/bsp_rtc.cpp"
         "src/bsp_modem.cpp"
         "src/bsp_sleep.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_driver_gpio esp_driver_i2c esp_driver_i2s esp_driver_ledc
             esp_driver_uart esp_hw_support
             esp_codec_dev button led_strip folotoy__folotoy-firmware-driver
)
