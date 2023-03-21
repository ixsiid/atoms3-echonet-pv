idf_component_register(SRCS "main.cpp" "button.cpp"
                    PRIV_REQUIRES nvs_flash WiFiManager esp-ECHONET-Lite esp32s3-adc
                    INCLUDE_DIRS ".")
