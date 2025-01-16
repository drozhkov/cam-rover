idf_component_register(
    SRCS _main.c helpers.c config.c wifi.c http.c discovery.c camera.c comm_udp.c drive.c
    INCLUDE_DIRS "."
    EMBED_FILES root.html
)
