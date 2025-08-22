idf_component_register(
    SRCS
        main.c
        global_var.c
        mainfunc.c
        gpio_all.c
        config.c
        wifi.c
        ble_srv.c
        mled.c
        ext_dev.c
        function.c
        otaflash.c
        mlist.c
        led_strip_encoder.c
        aes.c
        base64.c
        version.c
        mqttfunc.c
        devfunc.c
        dnsmulti.c
        dnsudp.c
        local_handle.c
    INCLUDE_DIRS "."        # optional, add here public include directories
    EMBED_TXTFILES ${project_dir}/key_tsall.pem
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements

)
