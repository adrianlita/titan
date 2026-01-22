#ifdef USE_ARTIFICIAL5
#include "a5_esp8266/a5_esp8266.h"
#include "a5_esp8266/a5_esp8266_internal.h"
#include "a5_errors/a5_errors.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define a5_esp8266_error_handle a5_error_handle

#ifdef A5_DEBUGGER_ACTIVE
#include "a5_debugger/a5_debugger.h"  //checkAL sterge-o la sfarsit
static uint8_t debug_channel = 0; //checkAL sterge-o la sfarsit
#endif

#define A5_ESP8266_TX_BUFFER_SIZE    64
#define A5_ESP8266_RX_BUFFER_SIZE    256
#define A5_ESP8266_RX_MAX_CMD_SIZE   128
#define A5_ESP8266_STATE_QUEUE_SIZE  10

#define A5_ESP8266_COMMAND_TIMEOUT   3  //second. timeout for command that is not recognized
#define A5_ESP8266_RXDATA_TIMEOUT    5  //second. timeout for receiving data. if data is not received in this interval, it is discarded

static uint8_t a5_esp8266_tx_buffer[A5_ESP8266_TX_BUFFER_SIZE];
static uint16_t a5_esp8266_tx_buffer_isr_index;
static uint16_t a5_esp8266_tx_buffer_prc_index;
static uint8_t a5_esp8266_tx_sending_data;
static uint8_t *a5_esp8266_tx_data_buffer;
static uint8_t *a5_esp8266_tx_data_buffer_end;
static uint8_t *a5_esp8266_tx_data_buffer_all_end;
static uint8_t a5_esp8266_tx_data_buffer_channel;
static uint8_t a5_esp8266_tx_sent_data;
static void(*a5_esp8266_tx_sent_data_callback)(const uint8_t success);

static uint8_t a5_esp8266_rx_buffer[A5_ESP8266_RX_BUFFER_SIZE];
static uint16_t a5_esp8266_rx_buffer_isr_index;
static uint16_t a5_esp8266_rx_buffer_prc_index;
static uint8_t a5_esp8266_rx_receiving_data;
static uint32_t a5_esp8266_rx_receiving_data_timeout;
static uint8_t a5_esp8266_rx_received_data;
static uint8_t a5_esp8266_rx_received_data_channel;

#define TIMEOUT_IN_SECONDS(x) (a5_esp8266_main_counter + x*2)
#define TIMEOUT_EXPIRED(timeout) (a5_esp8266_main_counter >= timeout)
static uint16_t a5_esp8266_isr_times_per_half_second = 0;
static uint32_t a5_esp8266_main_counter;

typedef enum
{
  STATE_ZERO = 0,

  STATE_INITIALIZING,
  STATE_TURN_ON,
  STATE_CHANGE_MODE,
  STATE_SETUP_AP,
  STATE_SETUP_STATION,

  STATE_CONNECT_AP,
  STATE_DISCONNECT_AP,
  STATE_LIST_APS,

  STATE_TCP_START_SERVER,
  STATE_TCP_STOP_SERVER,

  STATE_TCP_GET_STATUS,
  STATE_TCP_START_LINK,
  STATE_TCP_STOP_LINK,
  STATE_TCP_SEND_LINK,

  STATE_IDLE,

  STATE_TURN_OFF,

  STATE_NO_NEW_STATE
} _a5_esp8266_current_state;

static _a5_esp8266_current_state a5_esp8266_current_state;
static _a5_esp8266_current_state a5_esp8266_new_state[A5_ESP8266_STATE_QUEUE_SIZE];
static uint8_t a5_esp8266_new_state_in_index;
static uint8_t a5_esp8266_new_state_out_index;
static uint8_t a5_esp8266_new_state_full;

static uint8_t a5_esp8266_current_in_state;

static enum
{
  WIFI_DISCONNECTED = 0,
  WIFI_CONNECTED_WITHOUT_IP,
  WIFI_CONNECTED_WITH_IP
} a5_esp8266_wifi_status;

static _a5_esp8266_wifi_connect_fail_reason a5_esp8266_wifi_connect_fail_reason;

static _a5_esp8266_wifi_mode a5_esp8266_new_wifi_mode;
static _a5_esp8266_wifi_mode a5_esp8266_wifi_mode;
static uint8_t a5_esp8266_ip_mux;
static uint8_t a5_esp8266_ip_mode;

static _a5_esp8266_wifi_authentication_protocol a5_esp8266_ap_ecn;
static char a5_esp8266_ap_ssid_escaped[33];
static char a5_esp8266_ap_passkey_escaped[129];
static uint8_t a5_esp8266_ap_channel;
static uint8_t a5_esp8266_ap_hidden_ssid;
static uint32_t a5_esp8266_ap_ip_address;

static char a5_esp8266_station_ssid_escaped[33];
static char a5_esp8266_station_passkey_escaped[129];
static uint8_t a5_esp8266_station_autojoin;
static uint8_t a5_esp8266_station_dhcp_enabled;
static uint32_t a5_esp8266_station_ip_address;
static uint32_t a5_esp8266_station_netmask;
static uint32_t a5_esp8266_station_gateway;

static _a5_esp8266_wifi_connection_parameters_struct a5_esp8266_wifi_connection_parameters;
static _a5_esp8266_wifi_ap_list a5_esp8266_wifi_ap_list;
static _a5_esp8266_link_struct a5_esp8266_link[A5_ESP8266_MAX_LINKS];
static _a5_esp8266_link_struct a5_esp8266_link_temp[A5_ESP8266_MAX_LINKS];

static _a5_esp8266_server a5_esp8266_server;

static _a5_esp8266_link_struct a5_esp8266_connect_link;
static char a5_esp8266_connect_address_escaped[128];
static uint8_t a5_esp8266_disconnect_link_id;

static void(*a5_esp8266_on_module_on_callback)(const void* arg);
static void(*a5_esp8266_on_mode_change_callback)(const void* arg);
static void(*a5_esp8266_on_connect_success_callback)(const void* arg);
static void(*a5_esp8266_on_connect_fail_callback)(const void* arg);
static void(*a5_esp8266_on_disconnect_callback)(const void* arg);
static void(*a5_esp8266_on_ap_scan_complete_callback)(const void* arg);

static struct
{
  uint8_t ok;
  uint8_t error;
  uint8_t busy;

  uint8_t settings;
  uint8_t fail;
  uint8_t wifi_status;
  uint8_t no_change;

  uint8_t wifi_connect_failure;
  uint8_t wifi_ip;
  uint8_t wifi_ap_list;

  uint8_t send_ok;
  uint8_t send_fail;

  uint8_t link_urc;
  uint8_t already_connected;
  uint8_t unlink;
  uint8_t status;
} a5_esp8266_response;

static void a5_esp8266_internal_loop();
static void a5_esp8266_internal_process_message();
static void a5_esp8266_internal_add_new_state(_a5_esp8266_current_state x);

static uint8_t a5_esp8266_carret_received()
{
  if(a5_esp8266_rx_buffer_isr_index == 0)
  {
    return ((a5_esp8266_rx_buffer[A5_ESP8266_RX_BUFFER_SIZE - 2] == '>') && (a5_esp8266_rx_buffer[A5_ESP8266_RX_BUFFER_SIZE - 1] == ' '));
  }
  else
  if(a5_esp8266_rx_buffer_isr_index == 1)
  {
    return ((a5_esp8266_rx_buffer[A5_ESP8266_RX_BUFFER_SIZE - 1] == '>') && (a5_esp8266_rx_buffer[0] == ' '));
  }
  else
    return ((a5_esp8266_rx_buffer[a5_esp8266_rx_buffer_isr_index - 2] == '>') && (a5_esp8266_rx_buffer[a5_esp8266_rx_buffer_isr_index - 1] == ' '));
}

static void a5_esp8266_escape_chars(char* destination, const char* source)
{
  uint8_t i = 0;
  uint8_t k = 0;

  while(source[i] != 0)
  {
    if((source[i] == ',') || (source[i] == '"') || (source[i] == '\\'))
      destination[k++] = '\\';
    destination[k++] = source[i++];
  }
  destination[k] = 0;
}

void a5_esp8266_on_time_isr()
{
  static uint16_t counter;
  counter++;
  if(counter >= a5_esp8266_isr_times_per_half_second)
  {
    a5_esp8266_main_counter++;
    counter = 0;
  }
}

uint8_t a5_esp8266_on_tx_isr()
{
  uint8_t result;

  if(a5_esp8266_tx_sending_data)
  {
    if(a5_esp8266_tx_data_buffer == NULL)
    {
      a5_esp8266_error_handle(A5_ESP8266_ERROR_TX_DATA_BUFFER_NULL);
      result = 0;
    }
    else
    {
      result = *a5_esp8266_tx_data_buffer;
      a5_esp8266_tx_data_buffer++;
      if(a5_esp8266_tx_data_buffer >= a5_esp8266_tx_data_buffer_end)
      {
        a5_esp8266_tx_sending_data = 0;
      }
    }
  }
  else
  {
    result = (uint8_t)a5_esp8266_tx_buffer[a5_esp8266_tx_buffer_isr_index];
    a5_esp8266_tx_buffer_isr_index++;
    if(a5_esp8266_tx_buffer_isr_index >= A5_ESP8266_TX_BUFFER_SIZE)
      a5_esp8266_tx_buffer_isr_index = 0;
  }

  #ifdef A5_DEBUGGER_ACTIVE
  if(debug_channel)
    a5_debugger_printc(result);
  #endif
  return result;
}

int8_t a5_esp8266_is_tx_empty()
{
  if(a5_esp8266_tx_sending_data)
    return 0;
  else
    return (a5_esp8266_tx_buffer_isr_index == a5_esp8266_tx_buffer_prc_index);
}

void a5_esp8266_on_rx_isr(const uint8_t c)
{
  static char five_buffer[5] = {0, 0, 0, 0, 0};
  static uint8_t rx_data_chan;
  static uint8_t rx_data_chan_received;
  static uint16_t rx_data_len;
  static uint8_t rx_data_len_received;
  static uint16_t rx_data_count;

  #ifdef A5_DEBUGGER_ACTIVE
  if(debug_channel)
    a5_debugger_printc(c);
  #endif
  

  if(a5_esp8266_rx_receiving_data && (TIMEOUT_EXPIRED(a5_esp8266_rx_receiving_data_timeout)))
  {
    a5_esp8266_rx_receiving_data = 0;  
  }

  if(!a5_esp8266_rx_receiving_data)
  {
    five_buffer[0] = five_buffer[1];
    five_buffer[1] = five_buffer[2];
    five_buffer[2] = five_buffer[3];
    five_buffer[3] = five_buffer[4];
    five_buffer[4] = (char)c;

    if((five_buffer[0] == '+') && (five_buffer[1] == 'I') && (five_buffer[2] == 'P') && (five_buffer[3] == 'D') && (five_buffer[4] == ','))
    {
      a5_esp8266_rx_receiving_data = 1;
      a5_esp8266_rx_receiving_data_timeout = TIMEOUT_IN_SECONDS(A5_ESP8266_RXDATA_TIMEOUT);
      rx_data_chan = 0;
      rx_data_chan_received = 0;
      rx_data_len = 0;
      rx_data_len_received = 0;
    }

    a5_esp8266_rx_buffer[a5_esp8266_rx_buffer_isr_index] = c;
    a5_esp8266_rx_buffer_isr_index++;
    if(a5_esp8266_rx_buffer_isr_index >= A5_ESP8266_RX_BUFFER_SIZE)
      a5_esp8266_rx_buffer_isr_index = 0;
  }
  else
  {
    if(!rx_data_chan_received)
    {
      //channel unknown yet
      if(c != ',')
      {
        rx_data_chan *= 10;
        rx_data_chan += (c - '0');
      }
      else
      {
        rx_data_chan_received = 1;
      }
    }
    else
    {
      //channel known
      if(!rx_data_len_received)
      {
        //receiving data length
        if(c != ':')
        {
          rx_data_len *= 10;
          rx_data_len += (c - '0');
        }
        else
        {
          rx_data_len_received = 1;
          rx_data_count = rx_data_len;
          if(a5_esp8266_link[rx_data_chan].direction == A5_ESP8266_DIRECTION_SERVER)
          {
            a5_esp8266_link[rx_data_chan].rx_buffer = a5_esp8266_server.rx_buffer;
            a5_esp8266_link[rx_data_chan].rx_buffer_end = a5_esp8266_server.rx_buffer_end;
          }
        }
      }
      else
      {
        //receiving data
        if(a5_esp8266_link[rx_data_chan].rx_buffer != NULL)
        {
          *a5_esp8266_link[rx_data_chan].rx_buffer = c;
          a5_esp8266_link[rx_data_chan].rx_buffer++;
          if(a5_esp8266_link[rx_data_chan].rx_buffer >= a5_esp8266_link[rx_data_chan].rx_buffer_end)
          {
            //buffer full
            a5_esp8266_error_handle(A5_ESP8266_ERROR_RX_DATA_BUFFER_FULL);
            a5_esp8266_link[rx_data_chan].rx_buffer--;
          }
          else
            *a5_esp8266_link[rx_data_chan].rx_buffer = 0;
        }

        rx_data_count--;
        if(rx_data_count == 0)
        {
          if(a5_esp8266_link[rx_data_chan].rx_buffer == NULL)
          {
            a5_esp8266_error_handle(A5_ESP8266_ERROR_RX_DATA_BUFFER_NULL);
          }
          a5_esp8266_rx_receiving_data = 0;
          a5_esp8266_link[rx_data_chan].rx_data_length = rx_data_len;
          a5_esp8266_rx_received_data = 1;
          a5_esp8266_rx_received_data_channel = rx_data_chan;
          if(a5_esp8266_link[rx_data_chan].direction == A5_ESP8266_DIRECTION_SERVER)
          {
            a5_esp8266_server.rx_buffer = a5_esp8266_link[rx_data_chan].rx_buffer;
            a5_esp8266_server.rx_buffer_end = a5_esp8266_link[rx_data_chan].rx_buffer_end;
          }
        }
      } //end receiving data
    } //channel known
  } //a5_esp8266_rx_receiving_data == 1
}

static void a5_esp8266_put_tx(const char* s)
{
  uint8_t i = 0;
  if(s == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_TX_DATA_COMMAND_NULL);
    return;
  }

  while(s[i] != 0)
  {
    a5_esp8266_tx_buffer[a5_esp8266_tx_buffer_prc_index] = s[i];
    i++;
    a5_esp8266_tx_buffer_prc_index++;
    if(a5_esp8266_tx_buffer_prc_index >= A5_ESP8266_TX_BUFFER_SIZE)
      a5_esp8266_tx_buffer_prc_index = 0;
  }

  a5_esp8266_hw_start_sending();
}

static void a5_esp8266_internal_add_new_state(_a5_esp8266_current_state x)
{
  a5_esp8266_new_state[a5_esp8266_new_state_in_index] = x;
  a5_esp8266_new_state_in_index++;
  if(a5_esp8266_new_state_in_index >= A5_ESP8266_STATE_QUEUE_SIZE)
    a5_esp8266_new_state_in_index = 0;

  if(a5_esp8266_new_state_in_index == a5_esp8266_new_state_out_index)
    a5_esp8266_new_state_full = 1;
}

void a5_esp8266_init(const uint16_t isr_times_per_second)
{
  uint8_t i;

  a5_esp8266_tx_buffer_prc_index = 0;
  a5_esp8266_tx_buffer_isr_index = 0;
  a5_esp8266_tx_sending_data = 0;
  a5_esp8266_tx_data_buffer = NULL;
  a5_esp8266_tx_data_buffer_end = NULL;
  a5_esp8266_tx_data_buffer_all_end = NULL;
  a5_esp8266_tx_data_buffer_channel = 255;
  a5_esp8266_tx_sent_data = 1;
  a5_esp8266_tx_sent_data_callback = NULL;

  a5_esp8266_rx_buffer_prc_index = 0;
  a5_esp8266_rx_buffer_isr_index = 0;
  a5_esp8266_rx_receiving_data = 0;
  a5_esp8266_rx_received_data = 0;
  a5_esp8266_rx_received_data_channel = 255;

  a5_esp8266_current_state = STATE_ZERO;
  a5_esp8266_new_state_in_index = 0;
  a5_esp8266_new_state_out_index = 0;
  a5_esp8266_new_state_full = 0;
  a5_esp8266_new_state[0] = STATE_NO_NEW_STATE;

  if(isr_times_per_second < 2)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_ISR_TOO_SLOW);
  }

  a5_esp8266_isr_times_per_half_second = isr_times_per_second / 2;

  a5_esp8266_wifi_status = WIFI_DISCONNECTED;
  a5_esp8266_wifi_connect_fail_reason = A5_ESP8266_CONNECT_UNKNOWN_FAILURE;
  a5_esp8266_wifi_mode = A5_ESP8266_WIFI_MODE_UNKNOWN;
  a5_esp8266_ip_mux = 10;
  a5_esp8266_ip_mode = 10;

  strcpy(a5_esp8266_ap_ssid_escaped, "axiap");
  strcpy(a5_esp8266_ap_passkey_escaped, "");
  a5_esp8266_ap_ecn = A5_ESP8266_OPEN;
  a5_esp8266_ap_channel = 4;
  a5_esp8266_ap_hidden_ssid = 1;
  a5_esp8266_ap_ip_address = a5_esp8266_pack_ip_address(192,168,5,1);

  a5_esp8266_station_ssid_escaped[0] = 0;
  a5_esp8266_station_passkey_escaped[0] = 0;
  a5_esp8266_station_autojoin = 0;
  a5_esp8266_station_dhcp_enabled = 1;
  a5_esp8266_station_ip_address = 0;
  a5_esp8266_station_netmask = 0;
  a5_esp8266_station_gateway = 0;

  a5_esp8266_wifi_connection_parameters.ssid[0] = 0;
  a5_esp8266_wifi_connection_parameters.bssid[0] = 0;
  a5_esp8266_wifi_connection_parameters.channel = -1;
  a5_esp8266_wifi_connection_parameters.rssi = -255;
  a5_esp8266_wifi_connection_parameters.ip_address = 0;
  a5_esp8266_wifi_connection_parameters.netmask = 0;
  a5_esp8266_wifi_connection_parameters.gateway = 0;

  a5_esp8266_wifi_ap_list.aps_found = 0;

  a5_esp8266_response.ok = 0;
  a5_esp8266_response.error = 0;
  a5_esp8266_response.settings = 0;
  a5_esp8266_response.fail = 0;
  a5_esp8266_response.busy = 0;
  a5_esp8266_response.wifi_status = 0;
  a5_esp8266_response.wifi_connect_failure = 0;
  a5_esp8266_response.wifi_ip = 0;
  a5_esp8266_response.wifi_ap_list = 0;
  a5_esp8266_response.link_urc = 0;
  a5_esp8266_response.send_ok = 0;
  a5_esp8266_response.send_fail = 0;
  a5_esp8266_response.no_change = 0;
  a5_esp8266_response.already_connected = 0;
  a5_esp8266_response.unlink = 0;
  a5_esp8266_response.status = 0;


  for(i = 0; i < A5_ESP8266_MAX_LINKS; i++)
  {
    a5_esp8266_link[i].link_id = i;
    a5_esp8266_link[i].connected = 0;
    a5_esp8266_link[i].on_connected = NULL;
    a5_esp8266_link[i].on_receive = NULL;
    a5_esp8266_link[i].rx_buffer = NULL;
    a5_esp8266_link[i].rx_buffer_end = NULL;
    a5_esp8266_link[i].rx_data_length = 0;
    a5_esp8266_link[i].direction = A5_ESP8266_DIRECTION_UNKNOWN;
    a5_esp8266_link[i].close = 1; //close anything opened in the beginning
    a5_esp8266_link_temp[i] = a5_esp8266_link[i];
  }

  a5_esp8266_server.open = 0;

  a5_esp8266_connect_link = a5_esp8266_link[0];
  a5_esp8266_connect_link.link_id = 255;
  a5_esp8266_connect_address_escaped[0] = 0;
  a5_esp8266_disconnect_link_id = 255;
  
  a5_esp8266_on_module_on_callback = NULL;
  a5_esp8266_on_mode_change_callback = NULL;
  a5_esp8266_on_connect_success_callback = NULL;
  a5_esp8266_on_connect_fail_callback = NULL;
  a5_esp8266_on_disconnect_callback = NULL;
  a5_esp8266_on_ap_scan_complete_callback = NULL;
}

void a5_esp8266_main_loop()
{
  if(a5_esp8266_current_state == STATE_ZERO)
    return;

  if(!a5_esp8266_is_tx_empty() && !a5_esp8266_hw_send_busy())
  {
    a5_esp8266_hw_start_sending();
  }

  a5_esp8266_internal_process_message();

  if(a5_esp8266_rx_received_data)
  {
    if(a5_esp8266_link[a5_esp8266_rx_received_data_channel].on_receive)
      a5_esp8266_link[a5_esp8266_rx_received_data_channel].on_receive(&a5_esp8266_link[a5_esp8266_rx_received_data_channel]);

    a5_esp8266_rx_received_data = 0;
    a5_esp8266_rx_received_data_channel = 255;
  }

  a5_esp8266_internal_loop();
}

void a5_esp8266_enable_callback(const _a5_esp8266_callback callback, void(*function)(const void*))
{
  switch(callback)
  {
    case A5_ESP8266_ON_MODULE_ON:
      a5_esp8266_on_module_on_callback = function;
      break;

    case A5_ESP8266_ON_MODE_CHANGE:
      a5_esp8266_on_mode_change_callback = function;
      break;

    case A5_ESP8266_ON_CONNECT_SUCCESS:
      a5_esp8266_on_connect_success_callback = function;
      break;

    case A5_ESP8266_ON_CONNECT_FAIL:
      a5_esp8266_on_connect_fail_callback = function;
      break;

    case A5_ESP8266_ON_DISCONNECT:
      a5_esp8266_on_disconnect_callback = function;
      break;

    case A5_ESP8266_ON_AP_SCAN_COMPLETE:
      a5_esp8266_on_ap_scan_complete_callback = function;
      break;
  }
}

uint32_t a5_esp8266_pack_ip_address(const uint8_t byte1, const uint8_t byte2, const uint8_t byte3, const uint8_t byte4)
{
  uint32_t result = 0;
  result = (byte1 << 24) + (byte2 << 16) + (byte3 << 8) + byte4;
  return result;
}

void a5_esp8266_unpack_ip_address(const uint32_t ip_address, uint8_t* byte1, uint8_t* byte2, uint8_t* byte3, uint8_t* byte4)
{
  uint32_t buffer = ip_address;
  *byte4 = buffer & 0xFF;
  buffer >>= 8;
  *byte3 = buffer & 0xFF;
  buffer >>= 8;
  *byte2 = buffer & 0xFF;
  buffer >>= 8;
  *byte1 = buffer & 0xFF;
}

void a5_esp8266_set_ap_settings(const _a5_esp8266_wifi_authentication_protocol ecn, const char* ssid, const char* passkey, const uint8_t channel, const uint8_t ssid_hidden, const uint32_t ip_address)
{
  if((ssid == NULL) | (passkey == NULL))
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_NULL_SETTINGS);
    return;
  }

  if((strlen(ssid) > 32) || (strlen(ssid) == 0))
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SSID);
    return;
  }

  if((ecn == A5_ESP8266_WEP) || (ecn == A5_ESP8266_WPA2_Enterprise))
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_ECN);
    return;
  }

  if(strlen(passkey) != 0)
  {
    if((strlen(passkey) > 64) || (strlen(passkey) < 8))
    {
      a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_PASSKEY);
      return;
    }
  }
  else
  {
    if(ecn != A5_ESP8266_OPEN)
    {
      a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_PASSKEY);
      return;
    }
  }

  if((channel > 11) || (channel < 1))
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_AP_CHANNEL);
    return;
  }

  if((ip_address >> 24) != 192)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_IP);
    return;
  }

  //everything cool, now copy all and escape characters
  a5_esp8266_escape_chars(a5_esp8266_ap_ssid_escaped, ssid);
  a5_esp8266_escape_chars(a5_esp8266_ap_passkey_escaped, passkey);
  
  a5_esp8266_ap_ecn = ecn;
  a5_esp8266_ap_channel = channel;
  a5_esp8266_ap_hidden_ssid = ssid_hidden;
  a5_esp8266_ap_ip_address = ip_address;
}

void a5_esp8266_set_station_settings(const char* ssid, const char* passkey, const uint8_t join, const uint8_t dhcp, const uint32_t ip_address, const uint32_t netmask, const uint32_t gateway_address)
{
  if((ssid == NULL) | (passkey == NULL))
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_NULL_SETTINGS);
    return;
  }

  if((strlen(ssid) > 32) || (strlen(ssid) == 0))
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SSID);
    return;
  }

  if(strlen(passkey) != 0)
  {
    if((strlen(passkey) > 64) || (strlen(passkey) < 8))
    {
      a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_PASSKEY);
      return;
    }
  }

  //everything cool, now copy all and escape characters
  a5_esp8266_escape_chars(a5_esp8266_station_ssid_escaped, ssid);
  a5_esp8266_escape_chars(a5_esp8266_station_passkey_escaped, passkey);
  
  a5_esp8266_station_autojoin = join;
  a5_esp8266_station_dhcp_enabled = dhcp;
  if(!dhcp)
  {
    a5_esp8266_station_ip_address = ip_address;
    a5_esp8266_station_netmask = netmask;
    a5_esp8266_station_gateway = gateway_address;
  }
}

int8_t a5_esp8266_turn_on(const _a5_esp8266_wifi_mode initial_mode)
{
  if(a5_esp8266_current_state == STATE_ZERO)
  {
    a5_esp8266_new_wifi_mode = initial_mode;
    a5_esp8266_current_state = STATE_TURN_ON;
    return 0;
  }
  else
    return -1;
}

int8_t a5_esp8266_turn_off()
{
  a5_esp8266_current_state = STATE_TURN_OFF;
  return 0;
}

int8_t a5_esp8266_switch_mode(const _a5_esp8266_wifi_mode new_mode)
{
  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_new_wifi_mode = new_mode;
    a5_esp8266_internal_add_new_state(STATE_CHANGE_MODE);
    return 0;
  }
  else
    return -1;
}

int8_t a5_esp8266_scan_aps()
{
  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_internal_add_new_state(STATE_LIST_APS);
    return 0;
  }
  else
    return -1;
}

int8_t a5_esp8266_connect_to_ap()
{
  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_internal_add_new_state(STATE_CONNECT_AP);
    return 0;
  }
  else
    return -1;
}

int8_t a5_esp8266_disconnect_from_ap()
{
  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_internal_add_new_state(STATE_DISCONNECT_AP);
    return 0;
  }
  else
    return -1;
}

uint8_t a5_esp8266_is_connected_to_ap(_a5_esp8266_wifi_connection_parameters_struct* connection_parameters)
{
  if(a5_esp8266_wifi_status == WIFI_CONNECTED_WITH_IP)
  {
    *connection_parameters = a5_esp8266_wifi_connection_parameters;
    return 1;
  }
  else
  {
    return 0;
  }
}

int8_t a5_esp8266_server_open(const uint16_t port, void(*on_receive)(const void*), uint8_t* rx_buffer, const uint16_t rx_buffer_size)
{
  if(port < 10)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_PORT);
    return -1;
  }

  if(a5_esp8266_server.open)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_SERVER_ALREADY_OPEN);
    return 0;
  }

  if(rx_buffer == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_RX_DATA_BUFFER_NULL);
    return -1;
  }

  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_server.port = port;
    a5_esp8266_server.on_receive = on_receive;
    a5_esp8266_server.rx_buffer = rx_buffer;
    a5_esp8266_server.rx_buffer_end = rx_buffer + rx_buffer_size;
    a5_esp8266_internal_add_new_state(STATE_TCP_START_SERVER);
    return 0;
  }
  else
    return -1;
}

int8_t a5_esp8266_server_reset_rx_buffer(uint8_t* rx_buffer, const uint16_t rx_buffer_size)
{
  if(rx_buffer == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_RX_DATA_BUFFER_NULL);
    return -1;
  }

  a5_esp8266_server.rx_buffer = rx_buffer;
  a5_esp8266_server.rx_buffer_end = rx_buffer + rx_buffer_size;
  return 0;
}

int8_t a5_esp8266_server_close()
{
  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_internal_add_new_state(STATE_TCP_STOP_SERVER);
    return 0;
  }
  else
    return -1;
}

int8_t a5_esp8266_client_connect_tcp(const char* address, const uint16_t port, void(*on_connected)(const void*), void(*on_receive)(const void*), uint8_t* rx_buffer, const uint16_t rx_buffer_size)
{
  uint8_t id;
  uint8_t ok;
  
  if(address == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_NULL_ADDRESS);
    return -1;
  }

  if(port < 10)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_PORT);
    return -1;
  }

  if(rx_buffer == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_RX_DATA_BUFFER_NULL);
    return -1;
  }

  ok = 0;
  for(id = 0; id < A5_ESP8266_MAX_LINKS; id++)
  {
    if(a5_esp8266_link[id].connected == 0)
    {
      ok = 1;
      break;
    }
  }

  if(!ok)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_NO_FREE_LINK);
    return -1;
  }

  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_connect_link.link_id = id;
    a5_esp8266_connect_link.protocol = A5_ESP8266_PROTOCOL_TCP;
    a5_esp8266_escape_chars(a5_esp8266_connect_address_escaped, address);
    a5_esp8266_connect_link.direction = A5_ESP8266_DIRECTION_CLIENT;
    a5_esp8266_connect_link.remote_port = port;
    a5_esp8266_connect_link.on_connected = on_connected;
    a5_esp8266_connect_link.on_receive = on_receive;
    a5_esp8266_connect_link.rx_buffer = rx_buffer;
    a5_esp8266_connect_link.rx_buffer_end = rx_buffer + rx_buffer_size;

    a5_esp8266_internal_add_new_state(STATE_TCP_START_LINK);
    return id;
  }
  else
    return -1;
}

int8_t a5_esp8266_client_connect_udp(const char* address, const uint16_t port, void(*on_connected)(const void*), void(*on_receive)(const void*), uint8_t* rx_buffer, const uint16_t rx_buffer_size)
{
  uint8_t id;
  uint8_t ok;
  
  if(port < 10)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_PORT);
    return -1;
  }

  if(rx_buffer == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_RX_DATA_BUFFER_NULL);
    return -1;
  }

  ok = 0;
  for(id = 0; id < A5_ESP8266_MAX_LINKS; id++)
  {
    if(a5_esp8266_link[id].connected == 0)
    {
      ok = 1;
      break;
    }
  }

  if(!ok)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_NO_FREE_LINK);
    return -1;
  }

  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_connect_link.link_id = id;
    a5_esp8266_connect_link.protocol = A5_ESP8266_PROTOCOL_UDP;
    a5_esp8266_escape_chars(a5_esp8266_connect_address_escaped, address);
    a5_esp8266_connect_link.direction = A5_ESP8266_DIRECTION_CLIENT;
    a5_esp8266_connect_link.remote_port = port;
    a5_esp8266_connect_link.close = 0;
    a5_esp8266_connect_link.on_connected = on_connected;
    a5_esp8266_connect_link.on_receive = on_receive;
    a5_esp8266_connect_link.rx_buffer = rx_buffer;
    a5_esp8266_connect_link.rx_buffer_end = rx_buffer + rx_buffer_size;

    a5_esp8266_internal_add_new_state(STATE_TCP_START_LINK);
    return id;
  }
  else
    return -1;
}

int8_t a5_esp8266_client_reset_rx_buffer(const uint8_t link_id, uint8_t* rx_buffer, const uint16_t rx_buffer_size)
{
  if(rx_buffer == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_RX_DATA_BUFFER_NULL);
    return -1;
  }

  if(link_id >= A5_ESP8266_MAX_LINKS)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_LINK_ID);
    return -1;
  }

  a5_esp8266_link[link_id].rx_buffer = rx_buffer;
  a5_esp8266_link[link_id].rx_buffer_end = rx_buffer + rx_buffer_size;
  return 0;
}

int8_t a5_esp8266_is_client_connected(const uint8_t link_id)
{
  if(link_id >= A5_ESP8266_MAX_LINKS)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_LINK_ID);
    return -1;
  }

  return a5_esp8266_link[link_id].connected;
}

int8_t a5_esp8266_link_close(const uint8_t link_id)
{
  if(link_id >= A5_ESP8266_MAX_LINKS)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_LINK_ID);
    return -1;
  }

  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_link[link_id].close = 1;
    a5_esp8266_internal_add_new_state(STATE_TCP_GET_STATUS);
    return 0;
  }
  else
    return -1;
} 

int8_t a5_esp8266_send_data(const uint8_t link_id, uint8_t* tx_data, const uint16_t tx_data_size, void(*on_finish)(const uint8_t success))
{
  if(tx_data == NULL)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_TX_DATA_BUFFER_NULL);
    return -1;
  }

  if(tx_data_size == 0)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_TX_DATA_BUFFER_EMPTY);
    return -1;
  }

  if(link_id >= A5_ESP8266_MAX_LINKS)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_TX_DATA_BUFFER_EMPTY);
    return -1;
  }

  if(a5_esp8266_link[link_id].connected == 0)
  {
    a5_esp8266_error_handle(A5_ESP8266_ERROR_LINK_NOT_CONNECTED);
    return -1;
  }

  if(!a5_esp8266_new_state_full)
  {
    a5_esp8266_tx_data_buffer = tx_data;
    a5_esp8266_tx_data_buffer_all_end = tx_data + tx_data_size;
    a5_esp8266_tx_data_buffer_channel = link_id;
    a5_esp8266_tx_sent_data = 0;
    a5_esp8266_tx_sent_data_callback = on_finish;
    a5_esp8266_internal_add_new_state(STATE_TCP_SEND_LINK);
    return 0;
  }
  else
    return -1;
}

uint8_t a5_esp8266_is_sending_data()
{
  return (a5_esp8266_tx_sent_data == 0);
}

static void a5_esp8266_internal_process_message()
{
  static char two_buffer[2] = {0, 0};
  static char current_command[A5_ESP8266_RX_MAX_CMD_SIZE + 5];
  static uint8_t current_command_index = 0;
  static uint32_t current_command_timeout;
  uint8_t i, k;
  uint8_t p1, p2, p3, p4;
  uint8_t type;
  uint16_t port;

  while(a5_esp8266_rx_buffer_prc_index != a5_esp8266_rx_buffer_isr_index)
  {
    two_buffer[0] = two_buffer[1];
    two_buffer[1] = a5_esp8266_rx_buffer[a5_esp8266_rx_buffer_prc_index];

    if(a5_esp8266_main_counter > current_command_timeout)
      current_command_index = 0;
    current_command_timeout = TIMEOUT_IN_SECONDS(A5_ESP8266_COMMAND_TIMEOUT);

    current_command[current_command_index++] = a5_esp8266_rx_buffer[a5_esp8266_rx_buffer_prc_index];

    a5_esp8266_rx_buffer_prc_index++;
    if(a5_esp8266_rx_buffer_prc_index >= A5_ESP8266_RX_BUFFER_SIZE)
      a5_esp8266_rx_buffer_prc_index = 0;

      
    if(
      (current_command[0] == '+') &&
      (current_command[1] == 'I') &&
      (current_command[2] == 'P') &&
      (current_command[3] == 'D') &&
      (current_command[4] == ',')
      )
    {
      current_command_index = 0;
    }

    if((two_buffer[0] == '\r') && (two_buffer[1] == '\n'))
    {
      //command found
      current_command[current_command_index - 2] = 0;
      current_command_index = 0;

      if(strncmp(current_command, "OK", 2) == 0)
      {
        a5_esp8266_response.ok = 1;
      }
      else
      if(strncmp(current_command, "busy", 4) == 0)
      {
        a5_esp8266_response.busy = 1;
      }
      else
      if(strncmp(current_command, "ERROR", 5) == 0)
      {
        a5_esp8266_response.error = 1;
      }
      else
      if(strncmp(current_command, "+CWLAP:(", 8) == 0)
      {
        a5_esp8266_response.wifi_ap_list++;

        if(a5_esp8266_wifi_ap_list.aps_found < A5_ESP8266_MAX_AP_LIST)
        {
          switch(current_command[8])
          {
            case '0':
              a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ecn = A5_ESP8266_OPEN;
              break;

            case '1':
              a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ecn = A5_ESP8266_WEP;
              break;

            case '2':
              a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ecn = A5_ESP8266_WPA;
              break;

            case '3':
              a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ecn = A5_ESP8266_WPA2;
              break;

            case '4':
              a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ecn = A5_ESP8266_WPA_WPA2;
              break;

            case '5':
              a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ecn = A5_ESP8266_WPA2_Enterprise;
              break;

            default:
              a5_esp8266_error_handle(A5_ESP8266_ERROR_LIST_AP_PARSING);
              return;
              break;
          }

          if((current_command[9] != ',') && (current_command[10] != '"'))
          {
            a5_esp8266_error_handle(A5_ESP8266_ERROR_LIST_AP_PARSING);
            return;
          }

          i = 11;
          k = 0;
          while(current_command[i] != 0)
          {
            if(current_command[i] == '\\')
              i++;
            a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ssid[k++] = current_command[i++];

            if(current_command[i] == '"')
              break;
          }
          a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].ssid[k] = 0;

          i += 2;
          if(current_command[i] != '-')
          {
            a5_esp8266_error_handle(A5_ESP8266_ERROR_LIST_AP_PARSING);
            return;
          }
          i++;

          a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].rssi = 0;
          while((current_command[i] != 0) && (current_command[i] != ','))
          {
            a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].rssi *= 10;
            a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].rssi += current_command[i] - '0';
            i++;
          }
          a5_esp8266_wifi_ap_list.ap[a5_esp8266_wifi_ap_list.aps_found].rssi *= -1;

          a5_esp8266_wifi_ap_list.aps_found++;
        }
        
      }
      else
      if(strncmp(current_command, "STATUS:", 7) == 0)
      {
        a5_esp8266_response.status = 1;
      }
      else
      if(strncmp(current_command, "+CIPSTATUS:", 11) == 0)
      {
        a5_esp8266_response.settings = 1;
        k = current_command[11] - '0';
        if(k >= A5_ESP8266_MAX_LINKS)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }

        i = 14;
        switch(current_command[i])
        {
          case 'T':
            a5_esp8266_link_temp[k].protocol = A5_ESP8266_PROTOCOL_TCP;
            break;
          
          case 'U':
            a5_esp8266_link_temp[k].protocol = A5_ESP8266_PROTOCOL_UDP;
            break;

          default:
            a5_esp8266_link_temp[k].protocol = A5_ESP8266_PROTOCOL_UNKNOWN;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
            return;
            break;
        }

        while((current_command[i] != 0) && (current_command[i] != '"'))
          i++;

        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }
        
        i += 3;

        p1 = 0;
        while((current_command[i] != 0) && (current_command[i] != '.'))
        {
          p1 *= 10;
          p1 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }
        i++;

        p2 = 0;
        while((current_command[i] != 0) && (current_command[i] != '.'))
        {
          p2 *= 10;
          p2 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }
        i++;

        p3 = 0;
        while((current_command[i] != 0) && (current_command[i] != '.'))
        {
          p3 *= 10;
          p3 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }
        i++;

        p4 = 0;
        while((current_command[i] != 0) && (current_command[i] != '"'))
        {
          p4 *= 10;
          p4 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }

        a5_esp8266_link_temp[k].remote_ip = a5_esp8266_pack_ip_address(p1, p2, p3, p4);
        i += 2;

        port = 0;
        while((current_command[i] != 0) && (current_command[i] != ','))
        {
          port *= 10;
          port += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }
        a5_esp8266_link_temp[k].remote_port = port;
        i += 1;

        port = 0;
        while((current_command[i] != 0) && (current_command[i] != ','))
        {
          port *= 10;
          port += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
          return;
        }
        a5_esp8266_link_temp[k].local_port = port;

        i++;
        switch(current_command[i])
        {
          case '0':
            a5_esp8266_link_temp[k].direction = A5_ESP8266_DIRECTION_CLIENT;
            break;
          
          case '1':
            a5_esp8266_link_temp[k].direction = A5_ESP8266_DIRECTION_SERVER;
            break;

          default:
            a5_esp8266_link_temp[k].direction = A5_ESP8266_DIRECTION_UNKNOWN;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS_PARSING);
            return;
            break;
        }

        a5_esp8266_link_temp[k].connected = 1;
      }
      else
      if(strncmp(current_command, "SEND OK", 7) == 0)
      {
        a5_esp8266_response.send_ok = 1;
      }
      else
      if(strncmp(current_command, "ALREADY CONNECTED", 17) == 0)
      {
        a5_esp8266_response.already_connected = 1;
      }
      else
      if(strncmp(current_command + 1, "ALREADY CONNECTED", 17) == 0)
      {
        a5_esp8266_response.already_connected = 1;
      }
      else
      if(strncmp(current_command, "UNLINK", 6) == 0)
      {
        a5_esp8266_response.unlink = 1;
      }
      else
      if(strncmp(current_command, "SEND FAIL", 9) == 0)
      {
        a5_esp8266_response.send_fail = 1;
      }
      else
      if(strncmp(current_command, "no change", 9) == 0)
      {
        a5_esp8266_response.no_change = 1;
      }
      else
      if(strncmp(current_command + 1, ",CONNECT", 8) == 0)
      {
        
        k = current_command[0] - '0';
        if(k >= A5_ESP8266_MAX_LINKS)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_CONNECTCLOSE_PARSING);
          return;
        }
        a5_esp8266_response.link_urc = 1;
        a5_esp8266_link[k].connected = 1;
        a5_esp8266_link[k].close = 0;
        if(a5_esp8266_link[k].direction != A5_ESP8266_DIRECTION_CLIENT)
        {
          a5_esp8266_link[k].direction = A5_ESP8266_DIRECTION_SERVER;
          a5_esp8266_link[k].rx_buffer = a5_esp8266_server.rx_buffer;
          a5_esp8266_link[k].rx_buffer_end = a5_esp8266_server.rx_buffer_end;
          a5_esp8266_link[k].on_receive = a5_esp8266_server.on_receive;
        }
      }
      else
      if(strncmp(current_command + 1, ",CLOSED", 7) == 0)
      {
        k = current_command[0] - '0';
        if(k >= A5_ESP8266_MAX_LINKS)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_CONNECTCLOSE_PARSING);
          return;
        }
        a5_esp8266_response.link_urc = 1;
        a5_esp8266_link[k].connected = 0;
        a5_esp8266_link[k].protocol = A5_ESP8266_PROTOCOL_UNKNOWN;
        a5_esp8266_link[k].remote_ip = 0;
        a5_esp8266_link[k].remote_port = 0;
        a5_esp8266_link[k].local_port = 0;
        a5_esp8266_link[k].direction = A5_ESP8266_DIRECTION_UNKNOWN;
        a5_esp8266_link[k].close = 0;
      }
      else
      if(strncmp(current_command, "+CWMODE_CUR:", 12) == 0)
      {
        a5_esp8266_response.settings = 1;
        switch(current_command[12])
        {
          case '1':
            a5_esp8266_wifi_mode = A5_ESP8266_WIFI_MODE_STATION;
            break;

          case '2':
            a5_esp8266_wifi_mode = A5_ESP8266_WIFI_MODE_SOFTAP;
            break;

          case '3':
            a5_esp8266_wifi_mode = A5_ESP8266_WIFI_MODE_SOFTAP_AND_STATION;
            break;
          
          default:
            a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_CWMODE);
            return;
            break;
        }
      }
      else
      if(strncmp(current_command, "+CIPMODE:", 9) == 0)
      {
        a5_esp8266_response.settings = 1;
        switch(current_command[9])
        {
          case '0':
            a5_esp8266_ip_mode = 0;
            break;

          case '1':
            a5_esp8266_ip_mode = 1;
            break;

          default:
            a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_CIPMODE);
            return;
            break;
        }
      }
      else
      if(strncmp(current_command, "+CIPMUX:", 8) == 0)
      {
        a5_esp8266_response.settings = 1;
        switch(current_command[8])
        {
          case '0':
            a5_esp8266_ip_mux = 0;
            break;

          case '1':
            a5_esp8266_ip_mux = 1;
            break;

          default:
            a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_CIPMUX);
            return;
            break;
        }
      }
      else
      if(strncmp(current_command, "FAIL", 4) == 0)
      {
        a5_esp8266_response.fail = 1;
      }
      else
      if(strncmp(current_command, "+CWJAP:", 7) == 0)
      {
        a5_esp8266_response.wifi_connect_failure = 1;
        if(current_command[8] == 0)
        {
          //all fine
          switch(current_command[7])
          {
            default:
              a5_esp8266_wifi_connect_fail_reason = A5_ESP8266_CONNECT_UNKNOWN_FAILURE;
              break;

            case '2':
              a5_esp8266_wifi_connect_fail_reason = A5_ESP8266_CONNECT_WRONG_PASSWORD;
              break;

            case '3':
              a5_esp8266_wifi_connect_fail_reason = A5_ESP8266_CONNECT_SSID_NOT_FOUND;
              break;
          }
        }
      }
      else
      if(strncmp(current_command, "WIFI CONNECTED", 14) == 0)
      {
        a5_esp8266_response.wifi_status = 1;
        a5_esp8266_wifi_status = WIFI_CONNECTED_WITHOUT_IP;
      }
      else
      if(strncmp(current_command, "WIFI GOT IP", 11) == 0)
      {
        a5_esp8266_response.wifi_status = 1;
        a5_esp8266_wifi_status = WIFI_CONNECTED_WITH_IP;
      }
      else
      if(strncmp(current_command, "WIFI DISCONNECT", 15) == 0)
      {
        a5_esp8266_wifi_status = WIFI_DISCONNECTED;
        a5_esp8266_response.wifi_status = 1;
      }
      else
      if(strncmp(current_command, "No AP", 5) == 0)
      {
        a5_esp8266_wifi_status = WIFI_DISCONNECTED;
        a5_esp8266_response.wifi_status = 1;
      }
      else
      if(strncmp(current_command, "+CWJAP_CUR:", 11) == 0)
      {
        a5_esp8266_response.wifi_status = 1;
        a5_esp8266_wifi_status = WIFI_CONNECTED_WITHOUT_IP;
        
        if(current_command[11] != '"')
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }

        i = 12;
        k = 0;
        while(current_command[i] != 0)
        {
          if(current_command[i] == '\\')
            i++;
          a5_esp8266_wifi_connection_parameters.ssid[k++] = current_command[i++];

          if(current_command[i] == '"')
            break;
        }
        a5_esp8266_wifi_connection_parameters.ssid[k] = 0;

        if(current_command[i] != '"')
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i += 2;
        if(current_command[i] != '"')
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i++;

        k = 0;
        while((current_command[i] != '"') && (current_command[i] != 0))
        {
          a5_esp8266_wifi_connection_parameters.bssid[k++] = current_command[i++];
        }
        a5_esp8266_wifi_connection_parameters.bssid[k] = 0;

        if(current_command[i] != '"')
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i += 2;

        a5_esp8266_wifi_connection_parameters.channel = 0;
        while((current_command[i] != 0) && (current_command[i] != ','))
        {
          a5_esp8266_wifi_connection_parameters.channel *= 10;
          a5_esp8266_wifi_connection_parameters.channel += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i++;

        if(current_command[i] != '-')
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i++;

        a5_esp8266_wifi_connection_parameters.rssi = 0;
        while(current_command[i] != 0)
        {
          a5_esp8266_wifi_connection_parameters.rssi *= 10;
          a5_esp8266_wifi_connection_parameters.rssi += current_command[i] - '0';
          i++;
        }
        a5_esp8266_wifi_connection_parameters.rssi *= -1;
      }
      else
      if(strncmp(current_command, "+CIPSTA_CUR:", 12) == 0)
      {
        a5_esp8266_response.wifi_ip = 1;
        a5_esp8266_wifi_status = WIFI_CONNECTED_WITH_IP;
        
        switch(current_command[12])
        {
          case 'i':
          case 'g':
          case 'n':
            type = current_command[12];
            break;
          default:
            a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
            return;
            break;
        }
        i = 13;
        while((current_command[i] != 0) && (current_command[i] != '"'))
          i++;

        if(current_command[i] != '"')
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i++;

        p1 = 0;
        while((current_command[i] != 0) && (current_command[i] != '.'))
        {
          p1 *= 10;
          p1 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i++;

        p2 = 0;
        while((current_command[i] != 0) && (current_command[i] != '.'))
        {
          p2 *= 10;
          p2 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i++;

        p3 = 0;
        while((current_command[i] != 0) && (current_command[i] != '.'))
        {
          p3 *= 10;
          p3 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }
        i++;

        p4 = 0;
        while((current_command[i] != 0) && (current_command[i] != '"'))
        {
          p4 *= 10;
          p4 += current_command[i] - '0';
          i++;
        }
        if(current_command[i] == 0)
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
          return;
        }

        switch(type)
        {
          case 'i':
            a5_esp8266_wifi_connection_parameters.ip_address = a5_esp8266_pack_ip_address(p1, p2, p3, p4);
            break;

          case 'g':
            a5_esp8266_wifi_connection_parameters.gateway = a5_esp8266_pack_ip_address(p1, p2, p3, p4);
            break;

          case 'n':
            a5_esp8266_wifi_connection_parameters.netmask = a5_esp8266_pack_ip_address(p1, p2, p3, p4);
            break;

          default:
            a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_PARSING);
            return;
            break;
        }
      }
    }
  }
}

static void a5_esp8266_internal_loop()
{
  static uint32_t timeout;
  char aux[164];
  uint8_t p1, p2, p3, p4;
  uint16_t size;

  switch(a5_esp8266_current_state)
  {
    case STATE_ZERO:
    {
      //nothing. turn_on() will handle it
    } break;

    case STATE_TURN_ON:
    {
      a5_esp8266_hw_reset_off();
      a5_esp8266_hw_enable_on();
      a5_esp8266_current_in_state = 0;
      a5_esp8266_current_state = STATE_INITIALIZING;
    } break;

    case STATE_TURN_OFF:
    {
      a5_esp8266_hw_enable_off();
      a5_esp8266_hw_reset_on();
      a5_esp8266_current_state = STATE_ZERO;
    } break;

    case STATE_INITIALIZING:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          timeout = TIMEOUT_IN_SECONDS(2);
          a5_esp8266_current_in_state++;
        } break;

        case 1:
        {
          if(timeout <= a5_esp8266_main_counter)
          {
            a5_esp8266_current_in_state++;
            timeout = 0;
          }
        } break;

        case 2:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 3:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state++;
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 4:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("ATE0\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
          }
        } break;

        case 5:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state++;
            timeout = 0;
          }
        } break;

        case 6:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CWMODE_CUR?\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.settings = 0;
          }
        } break;

        case 7:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.settings)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            if(a5_esp8266_new_wifi_mode == a5_esp8266_wifi_mode)
              a5_esp8266_current_in_state += 3;
            else
              a5_esp8266_current_in_state++;

            timeout = 0;
          }
        } break;

        case 8:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            sprintf(aux, "AT+CWMODE_CUR=%d\r\n", a5_esp8266_new_wifi_mode);
            a5_esp8266_put_tx(aux);
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 9:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state -= 3;
            timeout = 0;
          }
        } break;

        case 10:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CIPMODE?\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.settings = 0;
          }
        } break;

        case 11:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.settings)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            if(a5_esp8266_ip_mode == 0)
              a5_esp8266_current_in_state += 3;
            else
              a5_esp8266_current_in_state++;

            timeout = 0;
          }
        } break;

        case 12:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CIPMODE=0\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 13:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state -= 3;
            timeout = 0;
          }
        } break;

        case 14:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CIPMUX?\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.settings = 0;
          }
        } break;

        case 15:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.settings)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            if(a5_esp8266_ip_mux == 1)
              a5_esp8266_current_in_state += 3;
            else
              a5_esp8266_current_in_state++;

            timeout = 0;
          }
        } break;

        case 16:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CIPMUX=1\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 17:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state -= 3;
            timeout = 0;
          }
        } break;

        case 18:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CIPDINFO=0\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 19:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state = 255;

            timeout = 0;
          }
        } break;

        case 255:
        {
          if(a5_esp8266_on_module_on_callback)
            a5_esp8266_on_module_on_callback(NULL);

          if(a5_esp8266_wifi_mode == A5_ESP8266_WIFI_MODE_STATION)
            a5_esp8266_current_state = STATE_SETUP_STATION;
          else
            a5_esp8266_current_state = STATE_SETUP_AP;

          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    }
    break;

    case STATE_CHANGE_MODE:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CWMODE_CUR?\r\n");
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.settings = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.settings)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            if(a5_esp8266_new_wifi_mode == a5_esp8266_wifi_mode)
            {
              a5_esp8266_current_in_state = 255;
            }
            else
              a5_esp8266_current_in_state++;

            timeout = 0;
          }
        } break;

        case 2:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            sprintf(aux, "AT+CWMODE_CUR=%d\r\n", a5_esp8266_new_wifi_mode);
            a5_esp8266_put_tx(aux);
            
            timeout = TIMEOUT_IN_SECONDS(1);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 3:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.settings = 0;
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state -= 3;
            timeout = 0;
          }
        } break;

        case 255:
        {
          if(a5_esp8266_on_mode_change_callback)
            a5_esp8266_on_mode_change_callback(&a5_esp8266_new_wifi_mode);

          if(a5_esp8266_wifi_mode == A5_ESP8266_WIFI_MODE_STATION)
            a5_esp8266_current_state = STATE_SETUP_STATION;
          else
            a5_esp8266_current_state = STATE_SETUP_AP;

          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_SETUP_AP:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            sprintf(aux, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,4,%d\r\n", a5_esp8266_ap_ssid_escaped, a5_esp8266_ap_passkey_escaped, a5_esp8266_ap_channel, (uint8_t)a5_esp8266_ap_ecn, a5_esp8266_ap_hidden_ssid);
            a5_esp8266_put_tx(aux);

            timeout = TIMEOUT_IN_SECONDS(3);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state++;
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_AP_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 2:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_unpack_ip_address(a5_esp8266_ap_ip_address, &p1, &p2, &p3, &p4);
            sprintf(aux, "AT+CIPAP_CUR=\"%d.%d.%d.%d\",\"%d.%d.%d.%d\",\"255.255.255.0\"\r\n", p1, p2, p3, p4, p1, p2, p3, p4);
            a5_esp8266_put_tx(aux);

            timeout = TIMEOUT_IN_SECONDS(2);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 3:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state = 255;
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_AP_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          a5_esp8266_current_state = STATE_SETUP_STATION;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }

    } break;

    case STATE_SETUP_STATION:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            if(a5_esp8266_station_dhcp_enabled)
              a5_esp8266_put_tx("AT+CWDHCP_CUR=1,1\r\n");
            else
              a5_esp8266_put_tx("AT+CWDHCP_CUR=1,0\r\n");

            timeout = TIMEOUT_IN_SECONDS(2);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            if(!a5_esp8266_station_dhcp_enabled)
              a5_esp8266_current_in_state++;
            else
              a5_esp8266_current_in_state = 4;
            
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 2:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_unpack_ip_address(a5_esp8266_station_ip_address, &p1, &p2, &p3, &p4);
            sprintf(aux, "AT+CIPSTA_CUR=\"%d.%d.%d.%d\",", p1, p2, p3, p4);

            a5_esp8266_unpack_ip_address(a5_esp8266_station_gateway, &p1, &p2, &p3, &p4);
            sprintf(aux + strlen(aux),"\"%d.%d.%d.%d\",", p1, p2, p3, p4);

            a5_esp8266_unpack_ip_address(a5_esp8266_station_netmask, &p1, &p2, &p3, &p4);
            sprintf(aux + strlen(aux),"\"%d.%d.%d.%d\"\r\n", p1, p2, p3, p4);

            a5_esp8266_put_tx(aux);
            timeout = TIMEOUT_IN_SECONDS(2);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 3:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state = 4;
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 4:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CWJAP_CUR?\r\n");
            timeout = TIMEOUT_IN_SECONDS(10);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.wifi_status = 0;
          }
        } break;

        case 5:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.wifi_status)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.wifi_status = 0;
            timeout = 0;

            a5_esp8266_current_in_state = 255;

            if(a5_esp8266_wifi_status == WIFI_DISCONNECTED)
            {
              if(a5_esp8266_on_disconnect_callback)
                a5_esp8266_on_disconnect_callback(NULL);
            }
            else
            {
              if(a5_esp8266_on_connect_success_callback)
                a5_esp8266_on_connect_success_callback(&a5_esp8266_wifi_connection_parameters);
            }
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_STATION_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          if(a5_esp8266_station_autojoin)
            a5_esp8266_current_state = STATE_CONNECT_AP;
          else
            a5_esp8266_current_state = STATE_TCP_GET_STATUS;
        
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_CONNECT_AP:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CWJAP_CUR?\r\n");
            timeout = TIMEOUT_IN_SECONDS(10);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.wifi_status = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.wifi_status)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.wifi_status = 0;
            timeout = 0;

            switch(a5_esp8266_wifi_status)
            {
              case WIFI_DISCONNECTED:
                a5_esp8266_current_in_state++;
                break;

              case WIFI_CONNECTED_WITHOUT_IP:
              case WIFI_CONNECTED_WITH_IP:
                a5_esp8266_current_in_state += 3;
                break;
            }  
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_CONNECT_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 2:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            sprintf(aux, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", a5_esp8266_station_ssid_escaped, a5_esp8266_station_passkey_escaped);
            a5_esp8266_put_tx(aux);
            timeout = TIMEOUT_IN_SECONDS(20);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.fail = 0;
          }
        } break;

        case 3:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state -= 3;
            timeout = 0;
          }
          else
          if(a5_esp8266_response.fail)
          {
            a5_esp8266_response.fail = 0;
            if(a5_esp8266_on_connect_fail_callback)
              a5_esp8266_on_connect_fail_callback(&a5_esp8266_wifi_connect_fail_reason);
            a5_esp8266_current_in_state = 255;
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_CONNECT_ERROR);

            if(a5_esp8266_on_connect_fail_callback)
              a5_esp8266_on_connect_fail_callback(&a5_esp8266_wifi_connect_fail_reason);

            a5_esp8266_current_in_state = 255;
          }
        } break;

        case 4:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CIPSTA_CUR?\r\n");
            timeout = TIMEOUT_IN_SECONDS(2);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.wifi_ip = 0;
          }
        } break;

        case 5:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.wifi_ip)
          {
            a5_esp8266_response.wifi_ip = 0;
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state = 255;
            if(a5_esp8266_on_connect_success_callback)
              a5_esp8266_on_connect_success_callback(&a5_esp8266_wifi_connection_parameters);
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_CONNECT_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          a5_esp8266_current_state = STATE_TCP_GET_STATUS;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_DISCONNECT_AP:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CWJAP_CUR?\r\n");
            timeout = TIMEOUT_IN_SECONDS(5);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.wifi_status = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.wifi_status)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.wifi_status = 0;
            timeout = 0;

            if(a5_esp8266_wifi_status == WIFI_DISCONNECTED)
              a5_esp8266_current_in_state = 255;
            else
              a5_esp8266_current_in_state++;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_DISCONNECT_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 2:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_put_tx("AT+CWQAP\r\n");
            timeout = TIMEOUT_IN_SECONDS(10);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 3:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_current_in_state -= 3;
            timeout = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_DISCONNECT_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          if(a5_esp8266_on_disconnect_callback)
            a5_esp8266_on_disconnect_callback(NULL);
          a5_esp8266_current_state = STATE_IDLE;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_LIST_APS:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_wifi_ap_list.aps_found = 0;
            a5_esp8266_put_tx("AT+CWLAP\r\n");
            timeout = TIMEOUT_IN_SECONDS(10);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.wifi_ap_list = 0;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok)
          {
            a5_esp8266_response.ok = 0;
            if(a5_esp8266_response.wifi_ap_list != a5_esp8266_wifi_ap_list.aps_found)
            {
              a5_esp8266_error_handle(A5_ESP8266_ERROR_LIST_AP_SMALL_BUFFER);  
            }

            a5_esp8266_response.wifi_ap_list = 0;
            timeout = 0;

            a5_esp8266_current_in_state = 255;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_LIST_AP_ERROR);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          if(a5_esp8266_on_ap_scan_complete_callback)
            a5_esp8266_on_ap_scan_complete_callback(&a5_esp8266_wifi_ap_list);
          a5_esp8266_current_state = STATE_IDLE;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_TCP_START_SERVER:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            sprintf(aux, "AT+CIPSERVER=1,%d\r\n", a5_esp8266_server.port);
            a5_esp8266_put_tx(aux);
            timeout = TIMEOUT_IN_SECONDS(3);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.no_change = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok || a5_esp8266_response.no_change)
          {
            a5_esp8266_response.no_change = 0;
            a5_esp8266_response.ok = 0;
            timeout = 0;
            a5_esp8266_server.open = 1;

            a5_esp8266_current_in_state = 255;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          a5_esp8266_current_state = STATE_IDLE;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_TCP_STOP_SERVER:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            sprintf(aux, "AT+CIPSERVER=0\r\n");
            a5_esp8266_put_tx(aux);
            timeout = TIMEOUT_IN_SECONDS(3);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.no_change = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok || a5_esp8266_response.no_change)
          {
            a5_esp8266_response.no_change = 0;
            a5_esp8266_response.ok = 0;
            timeout = 0;

            a5_esp8266_server.open = 0;
          
            a5_esp8266_current_in_state = 255;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          a5_esp8266_current_state = STATE_IDLE;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_TCP_GET_STATUS:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            for(uint8_t i = 0; i < A5_ESP8266_MAX_LINKS; i++)
            {
              a5_esp8266_link_temp[i] = a5_esp8266_link[i];
              a5_esp8266_link_temp[i].connected = 0;
            }

            a5_esp8266_put_tx("AT+CIPSTATUS\r\n");
            timeout = TIMEOUT_IN_SECONDS(2);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.status = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.status)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.status = 0;

            if(a5_esp8266_response.settings)
            {
              a5_esp8266_response.settings = 0;
            }

            for(uint8_t i = 0; i < A5_ESP8266_MAX_LINKS; i++)
            {
              a5_esp8266_link[i] = a5_esp8266_link_temp[i];
              if(a5_esp8266_link[i].connected == 0)
                a5_esp8266_link[i].close = 0;

              if((a5_esp8266_link[i].connected == 1) && (a5_esp8266_link[i].on_connected))
              {
                a5_esp8266_link[i].on_connected(&a5_esp8266_link[i]);
                a5_esp8266_link[i].on_connected = NULL;
              }
            }

            timeout = 0;
            a5_esp8266_current_in_state = 255;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_STATUS);
            a5_esp8266_current_in_state--;
            timeout = TIMEOUT_IN_SECONDS(2);
          }
        } break;

        case 255:
        {
          a5_esp8266_disconnect_link_id = 255;
          for(uint8_t i = 0; i < A5_ESP8266_MAX_LINKS; i++)
          {
            if((a5_esp8266_link[i].connected == 1) && (a5_esp8266_link[i].close))
            {
              a5_esp8266_disconnect_link_id = i;
              break;
            }
          }

          if(a5_esp8266_disconnect_link_id == 255)
            a5_esp8266_current_state = STATE_IDLE;
          else
            a5_esp8266_current_state = STATE_TCP_STOP_LINK;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_TCP_START_LINK:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_link[a5_esp8266_connect_link.link_id] = a5_esp8266_connect_link;
            if(a5_esp8266_link[a5_esp8266_connect_link.link_id].protocol == A5_ESP8266_PROTOCOL_TCP)
            {
              sprintf(aux, "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n", a5_esp8266_connect_link.link_id, a5_esp8266_connect_address_escaped, a5_esp8266_link[a5_esp8266_connect_link.link_id].remote_port);
            }
            else
            {
              sprintf(aux, "AT+CIPSTART=%d,\"UDP\",\"%s\",%d\r\n", a5_esp8266_connect_link.link_id, a5_esp8266_connect_address_escaped, a5_esp8266_link[a5_esp8266_connect_link.link_id].remote_port);
            }

            a5_esp8266_put_tx(aux);
            timeout = TIMEOUT_IN_SECONDS(5);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_response.already_connected = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.link_urc)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.link_urc = 0;
            timeout = 0;
            
            a5_esp8266_current_state = STATE_TCP_GET_STATUS;
            a5_esp8266_current_in_state = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            if(a5_esp8266_response.already_connected)
            {
              a5_esp8266_response.already_connected = 0;
              timeout = 0;
              a5_esp8266_current_state = STATE_TCP_GET_STATUS;
              a5_esp8266_current_in_state = 0;
            }
            else
            {
              a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_CONNECT);
              a5_esp8266_current_in_state = 255;
              timeout = TIMEOUT_IN_SECONDS(2);
            }
          }
        } break;

        case 255:
        {
          a5_esp8266_current_state = STATE_IDLE;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_TCP_STOP_LINK:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            if(a5_esp8266_disconnect_link_id != 255)
            {
              sprintf(aux, "AT+CIPCLOSE=%d\r\n", a5_esp8266_disconnect_link_id);
              a5_esp8266_disconnect_link_id = 255;
              a5_esp8266_put_tx(aux);
              timeout = TIMEOUT_IN_SECONDS(5);
              a5_esp8266_current_in_state++;
              a5_esp8266_response.ok = 0;
              a5_esp8266_response.error = 0;
              a5_esp8266_response.unlink = 0;
            }
            else
            {
              a5_esp8266_current_in_state = 255;
            }
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state--;
          }
          else
          if(a5_esp8266_response.ok && a5_esp8266_response.link_urc)
          {
            a5_esp8266_response.ok = 0;
            a5_esp8266_response.link_urc = 0;
            timeout = 0;
            a5_esp8266_current_state = STATE_TCP_GET_STATUS;
            a5_esp8266_current_in_state = 0;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            if(a5_esp8266_response.unlink)
            {
              a5_esp8266_response.unlink = 0;
              timeout = 0;
              a5_esp8266_current_state = STATE_TCP_GET_STATUS;
              a5_esp8266_current_in_state = 0;
            }
            else
            {
              a5_esp8266_error_handle(A5_ESP8266_ERROR_TCP_DISCONNECT);
              a5_esp8266_current_in_state = 255;
              timeout = TIMEOUT_IN_SECONDS(2);
            }
          }
        } break;

        case 255:
        {
          a5_esp8266_current_state = STATE_IDLE;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_TCP_SEND_LINK:
    {
      switch(a5_esp8266_current_in_state)
      {
        case 0:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            size = a5_esp8266_tx_data_buffer_all_end - a5_esp8266_tx_data_buffer;
            if(size > 2048)
              size = 2048;
            a5_esp8266_tx_data_buffer_end = a5_esp8266_tx_data_buffer + size;

            sprintf(aux, "AT+CIPSEND=%d,%d\r\n", a5_esp8266_tx_data_buffer_channel, size);
            a5_esp8266_put_tx(aux);
            timeout = TIMEOUT_IN_SECONDS(2);
            a5_esp8266_current_in_state++;
            a5_esp8266_response.send_ok = 0;
            a5_esp8266_response.send_fail = 0;
            a5_esp8266_response.error = 0;
          }
        } break;

        case 1:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state = 255;
            a5_esp8266_tx_sent_data = 1;
            if(a5_esp8266_tx_sent_data_callback)
              a5_esp8266_tx_sent_data_callback(0);
            timeout = 0;
          }
          else
          if(a5_esp8266_carret_received())
          {
            timeout = 0;
            a5_esp8266_tx_sending_data = 1;
            a5_esp8266_current_in_state++;
          }
          else
          if(a5_esp8266_response.error)
          {
            a5_esp8266_response.error = 0;
            a5_esp8266_current_in_state = 255;
            a5_esp8266_tx_sent_data = 1;
            if(a5_esp8266_tx_sent_data_callback)
              a5_esp8266_tx_sent_data_callback(0);
            timeout = 0;
          }
        } break;

        case 2:
        {
          if(a5_esp8266_tx_sending_data == 0)
          {
            timeout = TIMEOUT_IN_SECONDS(5);
            a5_esp8266_current_in_state++;
          }
        } break;

        case 3:
        {
          if(TIMEOUT_EXPIRED(timeout))
          {
            a5_esp8266_current_in_state = 255;
            a5_esp8266_tx_sent_data = 1;
            if(a5_esp8266_tx_sent_data_callback)
              a5_esp8266_tx_sent_data_callback(0);
            timeout = 0;
          }
          else
          if(a5_esp8266_response.send_ok)
          {
            a5_esp8266_response.send_ok = 0;
            
            if(a5_esp8266_tx_data_buffer_end == a5_esp8266_tx_data_buffer_all_end)
            {
              timeout = 0;
              a5_esp8266_current_in_state = 255;
              a5_esp8266_tx_sent_data = 1;
              if(a5_esp8266_tx_sent_data_callback)
                a5_esp8266_tx_sent_data_callback(1);
            }
            else
            {
              a5_esp8266_current_in_state = 0;
              timeout = TIMEOUT_IN_SECONDS(2);
            }
          }
          else
          if(a5_esp8266_response.error || a5_esp8266_response.send_fail)
          {
            a5_esp8266_response.send_fail = 0;
            a5_esp8266_response.error = 0;
            a5_esp8266_current_in_state = 255;
            a5_esp8266_tx_sent_data = 1;
            if(a5_esp8266_tx_sent_data_callback)
              a5_esp8266_tx_sent_data_callback(0);
            timeout = 0;
          }
        } break;

        case 255:
        {
          a5_esp8266_current_state = STATE_IDLE;
          a5_esp8266_current_in_state = 0;
        } break;

        default:
        {
          a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_SUB_STATE);
          a5_esp8266_current_state = STATE_ZERO;
          a5_esp8266_current_in_state = 0;
        } break;
      }
    } break;

    case STATE_IDLE:
    {
      if(a5_esp8266_response.link_urc)
      {
        a5_esp8266_response.link_urc = 0;
        a5_esp8266_current_state = STATE_TCP_GET_STATUS;
        a5_esp8266_current_in_state = 0;
      }
      else
      if(a5_esp8266_rx_receiving_data && (TIMEOUT_EXPIRED(a5_esp8266_rx_receiving_data_timeout)))
      {
        a5_esp8266_rx_receiving_data = 0;  
      }
      else
      if(a5_esp8266_new_state_full || (a5_esp8266_new_state_in_index != a5_esp8266_new_state_out_index))
      {  
        a5_esp8266_current_state = a5_esp8266_new_state[a5_esp8266_new_state_out_index];
        a5_esp8266_new_state_out_index++;
        if(a5_esp8266_new_state_out_index >= A5_ESP8266_STATE_QUEUE_SIZE)
          a5_esp8266_new_state_out_index = 0;

        a5_esp8266_new_state_full = 0;
        a5_esp8266_current_in_state = 0;
      }
    } break;

    default:
    {
      a5_esp8266_error_handle(A5_ESP8266_ERROR_INVALID_STATE);
      a5_esp8266_current_state = STATE_ZERO;
      a5_esp8266_current_in_state = 0;
    } break;
  }
}

#ifdef A5_DEBUGGER_ACTIVE
void a5_esp8266_debug_disable_at()
{
  debug_channel = 0;
}

void a5_esp8266_debug_function1()
{
  debug_channel = 1;
  a5_esp8266_internal_add_new_state(STATE_TCP_GET_STATUS);
}
#endif

#endif
