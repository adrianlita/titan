#ifdef USE_ARTIFICIAL5
#ifndef __a5_esp8266_h
#define __a5_esp8266_h

#include <stdint.h>

#define A5_ESP8266_MAX_AP_LIST      20
#define A5_ESP8266_MAX_LINKS        5   //as of datasheet: 0-4

typedef enum
{
  A5_ESP8266_WIFI_MODE_UNKNOWN = 0,
  A5_ESP8266_WIFI_MODE_STATION = 1,
  A5_ESP8266_WIFI_MODE_SOFTAP = 2,
  A5_ESP8266_WIFI_MODE_SOFTAP_AND_STATION = 3
} _a5_esp8266_wifi_mode;

typedef enum
{
  A5_ESP8266_OPEN = 0,
  A5_ESP8266_WEP = 1,
  A5_ESP8266_WPA = 2,
  A5_ESP8266_WPA2 = 3,
  A5_ESP8266_WPA_WPA2 = 4,
  A5_ESP8266_WPA2_Enterprise = 5
} _a5_esp8266_wifi_authentication_protocol;

typedef enum
{
  A5_ESP8266_CONNECT_UNKNOWN_FAILURE = 0,
  A5_ESP8266_CONNECT_WRONG_PASSWORD,
  A5_ESP8266_CONNECT_SSID_NOT_FOUND
} _a5_esp8266_wifi_connect_fail_reason;

typedef struct
{
  char ssid[33];
  char bssid[20];
  int8_t channel;
  int16_t rssi;
  uint32_t ip_address;
  uint32_t netmask;
  uint32_t gateway;
} _a5_esp8266_wifi_connection_parameters_struct;

typedef struct
{
  uint8_t aps_found;
  struct
  {
    _a5_esp8266_wifi_authentication_protocol ecn;
    char ssid[33];
    int16_t rssi;
  } ap[A5_ESP8266_MAX_AP_LIST];
} _a5_esp8266_wifi_ap_list;

typedef enum
{
  A5_ESP8266_PROTOCOL_UNKNOWN = 0,
  A5_ESP8266_PROTOCOL_TCP,
  A5_ESP8266_PROTOCOL_UDP
} _a5_esp8266_connection_protocol;

typedef enum
{
  A5_ESP8266_DIRECTION_UNKNOWN = 0,
  A5_ESP8266_DIRECTION_CLIENT,
  A5_ESP8266_DIRECTION_SERVER
} _a5_esp8266_connection_direction;
 
typedef struct
{
  uint8_t link_id;
  uint8_t connected;
  _a5_esp8266_connection_protocol protocol;
  uint32_t remote_ip;
  uint16_t remote_port;
  uint16_t local_port;
  _a5_esp8266_connection_direction direction;

  uint8_t* rx_buffer;
  uint8_t* rx_buffer_end;
  uint16_t rx_data_length;

  uint8_t close;

  void(*on_connected)(const void*);
  void(*on_receive)(const void*);
} _a5_esp8266_link_struct;

typedef struct
{
  uint8_t open;
  uint16_t port;
  uint8_t* rx_buffer;
  uint8_t* rx_buffer_end;
  void(*on_receive)(const void*);
} _a5_esp8266_server;

typedef enum
{
  A5_ESP8266_ON_MODULE_ON = 0,      //A5_ESP8266_ON_MODULE_ON(NULL)
  A5_ESP8266_ON_MODE_CHANGE,        //A5_ESP8266_ON_MODE_CHANGE(_a5_esp8266_wifi_mode *new_mode)
  A5_ESP8266_ON_CONNECT_SUCCESS,    //A5_ESP8266_ON_CONNECT_SUCCESS(_a5_esp8266_wifi_connection_paramters_struct* connection_parameters)
  A5_ESP8266_ON_CONNECT_FAIL,       //A5_ESP8266_ON_CONNECT_FAIL(_a5_esp8266_wifi_connect_fail_reason* reason)
  A5_ESP8266_ON_DISCONNECT,         //A5_ESP8266_ON_DISCONNECT(NULL)
  A5_ESP8266_ON_AP_SCAN_COMPLETE    //A5_ESP8266_ON_AP_SCAN_COMPLETE(_a5_esp8266_wifi_ap_list* ap_list)
} _a5_esp8266_callback;

void a5_esp8266_init(const uint16_t isr_times_per_second);
void a5_esp8266_main_loop();

void a5_esp8266_enable_callback(const _a5_esp8266_callback callback, void(*function)(const void*));

void a5_esp8266_set_ap_settings(const _a5_esp8266_wifi_authentication_protocol ecn, const char* ssid, const char* passkey, const uint8_t channel, const uint8_t ssid_hidden, const uint32_t ip_address);
void a5_esp8266_set_station_settings(const char* ssid, const char* passkey, const uint8_t join, const uint8_t dhcp, const uint32_t ip_address,  const uint32_t netmask, const uint32_t gateway_address);

//following functions return 0 on success, -1 on error (retry on error)
int8_t a5_esp8266_turn_on(const _a5_esp8266_wifi_mode initial_mode);
int8_t a5_esp8266_turn_off();

int8_t a5_esp8266_switch_mode(const _a5_esp8266_wifi_mode new_mode);

int8_t a5_esp8266_scan_aps();
int8_t a5_esp8266_connect_to_ap();
int8_t a5_esp8266_disconnect_from_ap();
uint8_t a5_esp8266_is_connected_to_ap(_a5_esp8266_wifi_connection_parameters_struct* connection_parameters);

int8_t a5_esp8266_server_open(const uint16_t port, void(*on_receive)(const void*), uint8_t* rx_buffer, const uint16_t rx_buffer_size);
int8_t a5_esp8266_server_reset_rx_buffer(uint8_t* rx_buffer, const uint16_t rx_buffer_size);
int8_t a5_esp8266_server_close();

int8_t a5_esp8266_client_connect_tcp(const char* address, const uint16_t port, void(*on_connected)(const void*), void(*on_receive)(const void*), uint8_t* rx_buffer, const uint16_t rx_buffer_size);  //returns link id or -1
int8_t a5_esp8266_client_connect_udp(const char* address, const uint16_t port, void(*on_connected)(const void*), void(*on_receive)(const void*), uint8_t* rx_buffer, const uint16_t rx_buffer_size);  //returns link id or -1
int8_t a5_esp8266_client_reset_rx_buffer(const uint8_t link_id, uint8_t* rx_buffer, const uint16_t rx_buffer_size);
int8_t a5_esp8266_is_client_connected(const uint8_t link_id);

int8_t a5_esp8266_link_close(const uint8_t link_id);  //closes both server and client links

int8_t a5_esp8266_send_data(const uint8_t link_id, uint8_t* tx_data, const uint16_t tx_data_size, void(*on_finish)(const uint8_t success));
uint8_t a5_esp8266_is_sending_data();

//helper functions
uint32_t a5_esp8266_pack_ip_address(const uint8_t byte1, const uint8_t byte2, const uint8_t byte3, const uint8_t byte4);
void a5_esp8266_unpack_ip_address(const uint32_t ip_address, uint8_t* byte1, uint8_t* byte2, uint8_t* byte3, uint8_t* byte4);

#ifdef A5_DEBUGGER_ACTIVE
void a5_esp8266_debug_disable_at(); //checkAL sterge-o la sfarsit
void a5_esp8266_debug_function1(); //checkAL sterge-o la sfarsit
#endif

#endif
#endif
