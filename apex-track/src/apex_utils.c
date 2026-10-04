#include "apex_utils.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static ntrip_config_t ntrip_config = {
    .host = NTRIP_HOST,
    .port = NTRIP_PORT,
    .mountpoint = NTRIP_MOUNTPOINT,
    .username = NTRIP_USERNAME,
    .password = NTRIP_PASSWORD,
    .client_name = NTRIP_CLIENT_NAME,
    .gga_interval_ms = NTRIP_GGA_INTERVAL_MS,
};

static size_t ntrip_base64_encode(const char *input, char *output, size_t output_size)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t input_length = strlen(input);
    size_t output_length = 0;

    for (size_t index = 0; index < input_length; index += 3) {
        uint32_t value = (uint32_t)(unsigned char)input[index] << 16;
        size_t remaining = input_length - index;
        if (remaining > 1) {
            value |= (uint32_t)(unsigned char)input[index + 1] << 8;
        }
        if (remaining > 2) {
            value |= (uint32_t)(unsigned char)input[index + 2];
        }
        if (output_length + 4 >= output_size) {
            return 0;
        }
        output[output_length++] = alphabet[(value >> 18) & 0x3f];
        output[output_length++] = alphabet[(value >> 12) & 0x3f];
        output[output_length++] = remaining > 1 ? alphabet[(value >> 6) & 0x3f] : '=';
        output[output_length++] = remaining > 2 ? alphabet[value & 0x3f] : '=';
    }

    output[output_length] = '\0';
    return output_length;
}

bool ntrip_send_all(int socket, const char *data, size_t length)
{
    while (length > 0) {
        int bytes_sent = send(socket, data, length, 0);
        if (bytes_sent <= 0) {
            return false;
        }
        data += bytes_sent;
        length -= bytes_sent;
    }
    return true;
}

int ntrip_connect(void)
{
    char port[8];
    snprintf(port, sizeof(port), "%u", ntrip_config.port);

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *address_list = NULL;
        if (getaddrinfo(ntrip_config.host, port, &hints, &address_list) != 0) {
		ESP_LOGE("ntrip", "Could not resolve caster %s", ntrip_config.host);
        return -1;
    }

    int socket_fd = -1;
    for (struct addrinfo *address = address_list; address != NULL; address = address->ai_next) {
        socket_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (socket_fd < 0 || connect(socket_fd, address->ai_addr, address->ai_addrlen) != 0) {
            if (socket_fd >= 0) {
                close(socket_fd);
            }
            socket_fd = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(address_list);

    if (socket_fd < 0) {
        ESP_LOGE("ntrip", "Could not connect to caster");
        return -1;
    }

    char credentials[128];
    char authorization[180];
    snprintf(credentials, sizeof(credentials), "%s:%s", ntrip_config.username, ntrip_config.password);
    if (ntrip_base64_encode(credentials, authorization, sizeof(authorization)) == 0) {
        close(socket_fd);
        return -1;
    }

    char request[512];
    int request_length = snprintf(
        request,
        sizeof(request),
        "GET /%s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Authorization: Basic %s\r\n"
        "Ntrip-Version: Ntrip/2.0\r\n"
        "Connection: keep-alive\r\n\r\n",
        ntrip_config.mountpoint,
        ntrip_config.host,
        ntrip_config.client_name,
        authorization);
    if (request_length < 0 || request_length >= sizeof(request) ||
        !ntrip_send_all(socket_fd, request, request_length)) {
        close(socket_fd);
        return -1;
    }

    struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char response[128] = {0};
    size_t response_length = 0;
    while (response_length < sizeof(response) - 1) {
        int bytes_read = recv(socket_fd, &response[response_length], 1, 0);
        if (bytes_read <= 0) {
            break;
        }
        response_length++;
        if (response_length >= 4 && memcmp(&response[response_length - 4], "\r\n\r\n", 4) == 0) {
            break;
        }
    }

    response[response_length] = '\0';
    if (response_length == 0 ||
        (strncmp(response, "ICY 200", 7) != 0 && strncmp(response, "HTTP/1.0 200", 12) != 0 &&
         strncmp(response, "HTTP/1.1 200", 12) != 0)) {
        ESP_LOGE("ntrip", "Caster rejected connection: %s", response);
        close(socket_fd);
        return -1;
    }

    ESP_LOGI("ntrip", "Connected to %s/%s", ntrip_config.host, ntrip_config.mountpoint);
    return socket_fd;
}

void ntrip_get_config(ntrip_config_t *config)
{
    if (config != NULL) {
        *config = ntrip_config;
    }
}

esp_err_t ntrip_set_config(const ntrip_config_t *config)
{
    if (config == NULL || config->host[0] == '\0' || config->mountpoint[0] == '\0' ||
        config->port == 0 || config->gga_interval_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    ntrip_config = *config;
    return ESP_OK;
}

esp_err_t ntrip_save_config(const ntrip_config_t *config)
{
    esp_err_t error = ntrip_set_config(config);
    if (error != ESP_OK) {
        return error;
    }
    nvs_handle_t handle;
    error = nvs_open("wifi_config", NVS_READWRITE, &handle);
    if (error != ESP_OK) {
        return error;
    }
    nvs_set_str(handle, "ntrip_host", ntrip_config.host);
    nvs_set_u16(handle, "ntrip_port", ntrip_config.port);
    nvs_set_str(handle, "ntrip_mount", ntrip_config.mountpoint);
    nvs_set_str(handle, "ntrip_user", ntrip_config.username);
    nvs_set_str(handle, "ntrip_pass", ntrip_config.password);
    nvs_set_str(handle, "ntrip_client", ntrip_config.client_name);
    nvs_set_u32(handle, "ntrip_interval", ntrip_config.gga_interval_ms);
    error = nvs_commit(handle);
    nvs_close(handle);
    return error;
}

esp_err_t ntrip_load_config(void)
{
    nvs_handle_t handle;
    esp_err_t error = nvs_open("wifi_config", NVS_READONLY, &handle);
    if (error != ESP_OK) {
        return error;
    }
    ntrip_config_t loaded = ntrip_config;
    size_t length;
    length = sizeof(loaded.host); nvs_get_str(handle, "ntrip_host", loaded.host, &length);
    length = sizeof(loaded.mountpoint); nvs_get_str(handle, "ntrip_mount", loaded.mountpoint, &length);
    length = sizeof(loaded.username); nvs_get_str(handle, "ntrip_user", loaded.username, &length);
    length = sizeof(loaded.password); nvs_get_str(handle, "ntrip_pass", loaded.password, &length);
    length = sizeof(loaded.client_name); nvs_get_str(handle, "ntrip_client", loaded.client_name, &length);
    nvs_get_u16(handle, "ntrip_port", &loaded.port);
    nvs_get_u32(handle, "ntrip_interval", &loaded.gga_interval_ms);
    nvs_close(handle);
    return ntrip_set_config(&loaded);
}

static uint8_t gps_command_checksum(const char *command)
{
    uint8_t checksum = 0;
    for (const char *character = command; *character != '\0'; character++) {
        checksum ^= (uint8_t)*character;
    }
    return checksum;
}

esp_err_t gps_wait_for_response(const char *command)
{
    char response[GPS_COMMAND_RESPONSE_BUFFER_SIZE];
    bool received_response = false;

    while (true) {
        int bytes_read = uart_read_bytes(
            GPS_UART_NUM,
            (uint8_t *)response,
            sizeof(response) - 1,
            pdMS_TO_TICKS(GPS_COMMAND_RESPONSE_TIMEOUT_MS));
        if (bytes_read <= 0) {
            break;
        }
        response[bytes_read] = '\0';
        ESP_LOGI("gps_wait_for_response", "GPS response: %s", response);
        received_response = true;
    }

    if (!received_response) {
        ESP_LOGW("gps_wait_for_response", "No response received for GPS command: %s", command);
    }
    return ESP_OK;
}

esp_err_t gps_send_command(const char *command, bool wait_for_response)
{
    const char *command_body = command;
    if (*command_body == '$') {
        command_body++;
    }

    char frame[128];
    int frame_length = snprintf(
        frame,
        sizeof(frame),
        "$%s*%02X\r\n",
        command_body,
        gps_command_checksum(command_body));
    if (frame_length < 0 || frame_length >= sizeof(frame)) {
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t error = uart_write_bytes(GPS_UART_NUM, frame, frame_length);
    if (error < 0) {
        return error;
    }
    error = uart_wait_tx_done(GPS_UART_NUM, pdMS_TO_TICKS(1000));
    if (error != ESP_OK) {
        return error;
    }
    return wait_for_response ? gps_wait_for_response(command) : ESP_OK;
}

bool gps_parse_gga_message(char *message, gps_data_t *gps_data)
{
    char *fields[16] = {0};
    size_t field_count = 0;

    for (char *field = message; field != NULL && field_count < 16;) {
        fields[field_count++] = field;
        char *separator = strchr(field, ',');
        if (separator == NULL) {
            break;
        }
        *separator = '\0';
        field = separator + 1;
    }

    if (field_count < 14 ||
        (strcmp(fields[0], "$GNGGA") != 0 && strcmp(fields[0], "$GPGGA") != 0) ||
        fields[1][0] == '\0' || fields[2][0] == '\0' || fields[3][0] == '\0' ||
        fields[4][0] == '\0' || fields[5][0] == '\0' || fields[6][0] == '\0' ||
        fields[9][0] == '\0') {
        return false;
    }

    int hours;
    int minutes;
    double seconds;
    double raw_latitude = strtod(fields[2], NULL);
    double raw_longitude = strtod(fields[4], NULL);
    if (sscanf(fields[1], "%2d%2d%lf", &hours, &minutes, &seconds) != 3 ||
        hours > 23 || minutes > 59 || seconds >= 60.0) {
        return false;
    }

    gps_data->utc_time_ms = ((uint32_t)hours * 3600U + (uint32_t)minutes * 60U +
                             (uint32_t)seconds) * 1000U +
                            (uint32_t)((seconds - (uint32_t)seconds) * 1000.0);
    gps_data->latitude = (int)(raw_latitude / 100.0) + fmod(raw_latitude, 100.0) / 60.0;
    gps_data->longitude = (int)(raw_longitude / 100.0) + fmod(raw_longitude, 100.0) / 60.0;
    gps_data->altitude_m = strtof(fields[9], NULL);
    gps_data->gps_mode = (uint8_t)strtoul(fields[6], NULL, 10);
    gps_data->correction_age_s = fields[13] != NULL && fields[13][0] != '\0'
        ? strtof(fields[13], NULL)
        : -1.0f;

    if (fields[3][0] == 'S') {
        gps_data->latitude = -gps_data->latitude;
    }
    if (fields[5][0] == 'W') {
        gps_data->longitude = -gps_data->longitude;
    }
    return true;
}

bool gps_parse_rmc_message(char *message, float *speed_kmh)
{
    char *fields[10] = {0};
    size_t field_count = 0;
    for (char *field = message; field != NULL && field_count < 10;) {
        fields[field_count++] = field;
        char *separator = strchr(field, ',');
        if (separator == NULL) {
            break;
        }
        *separator = '\0';
        field = separator + 1;
    }
    if (speed_kmh == NULL || field_count < 8 ||
        (strcmp(fields[0], "$GNRMC") != 0 && strcmp(fields[0], "$GPRMC") != 0) ||
        fields[2][0] != 'A' || fields[7][0] == '\0') {
        return false;
    }
    *speed_kmh = strtof(fields[7], NULL) * 1.852f;
    return true;
}
