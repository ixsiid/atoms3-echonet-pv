idf_component_register(SRCS "main.cpp" "button.cpp"
                    PRIV_REQUIRES nvs_flash WiFiManager esp-ECHONET-Lite
                    INCLUDE_DIRS ".")
