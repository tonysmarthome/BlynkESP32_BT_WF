/****************************************************************************************************************************
   BlynkSimpleESP32_WFM.h
   For ESP32 using WiFiManager and WiFi along with BlueTooth / BLE

   BlynkESP32_BT_WF is a library for inclusion of both ESP32 Blynk BT/BLE and WiFi libraries. Then select either one or both at runtime.
   Forked from Blynk library v0.6.1 https://github.com/blynkkk/blynk-library/releases
   Built by Khoi Hoang https://github.com/khoih-prog/BlynkGSM_ESPManager
   Licensed under MIT license
   Version: 1.0.4

   Original Blynk Library author:
   @file       BlynkSimpleESP32.h
   @author     Volodymyr Shymanskyy
   @license    This project is released under the MIT License (MIT)
   @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
   @date       Oct 2016
   @brief

   Version Modified By   Date      Comments
   ------- -----------  ---------- -----------
    1.0.0   K Hoang      25/01/2020 Initial coding
    1.0.1   K Hoang      27/01/2020 Enable simultaneously running BT/BLE and WiFi
    1.0.2   K Hoang      04/02/2020 Add Blynk WiFiManager support similar to Blynk_WM library
    1.0.3   K Hoang      24/02/2020 Add checksum, clearConfigData()
    1.0.4   K Hoang      14/03/2020 Enhance GUI. Reduce code size.
 *****************************************************************************************************************************/

#ifndef BlynkSimpleEsp32_WFM_h
#define BlynkSimpleEsp32_WFM_h

#ifndef ESP32
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#define BLYNK_SEND_ATOMIC

// KH
#define BLYNK_TIMEOUT_MS     30000UL

#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <Adapters/BlynkArduinoClient.h>
#include <WiFi.h>

#include <WebServer.h>

//default to use EEPROM, otherwise, use SPIFFS
#if USE_SPIFFS
#include <FS.h>
#include "SPIFFS.h"
#else
#include <EEPROM.h>
#endif

#include <esp_wifi.h>
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

// Configurable items besides fixed Header
#define NUM_CONFIGURABLE_ITEMS    8
typedef struct Configuration
{
  char header         [16];
  char wifi_ssid      [32];
  char wifi_pw        [32];
  char blynk_server   [32];
  int  blynk_port;
  char blynk_token    [36];
  char blynk_bt_tk    [36];
  char blynk_ble_tk   [36];
  char board_name     [24];
  int  checkSum;
} Blynk_WF_Configuration;

// Currently CONFIG_DATA_SIZE  =   252
uint16_t CONFIG_DATA_SIZE = sizeof(Blynk_WF_Configuration);

#define root_html_template "\
<!DOCTYPE html><html><head><title>Blynk_Esp32_BT_BLE_WF</title><style>.em{padding-bottom:0px;}div,input{padding:5px;font-size:1em;}input{width:95%;}\
body{text-align: center;}button{background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}fieldset{border-radius:0.3rem;margin:0px;}\
</style></head><div style=\"text-align:left;display:inline-block;min-width:260px;\">\
<fieldset><div><label>SSID</label><input value=\"[[id]]\"id=\"id\"><div></div></div><div><label>PWD</label><input value=\"[[pw]]\"id=\"pw\"><div></div></div></fieldset>\
<fieldset><div><label>Blynk Server</label><input value=\"[[sv]]\"id=\"sv\"><div></div></div><div><label>Port</label><input type=\"number\"value=\"[[pt]]\"id=\"pt\"><div></div></div>\
<div><label>WiFiToken</label><input value=\"[[tk]]\"id=\"tk\"><div></div></div><div><label>BT Token</label><input value=\"[[tk1]]\"id=\"tk1\"><div></div></div>\
<div><label>BLE Token</label><input value=\"[[tk2]]\"id=\"tk2\"><div></div></div></fieldset>\
<fieldset><div><label>Name</label><input value=\"[[nm]]\"id=\"nm\"><div></div></div></fieldset><button onclick=\"sv()\">Save</button></div><script id=\"jsbin-javascript\">\
function udVal(key,val){var request=new XMLHttpRequest();var url='/?key='+key+'&value='+val;request.open('GET',url,false);request.send(null);}\
function sv(){udVal('id',document.getElementById('id').value);udVal('pw',document.getElementById('pw').value);udVal('sv',document.getElementById('sv').value);\
udVal('pt',document.getElementById('pt').value);udVal('tk',document.getElementById('tk').value);udVal('tk1',document.getElementById('tk1').value);\
udVal('tk2',document.getElementById('tk2').value);udVal('nm',document.getElementById('nm').value);alert('Updated');}</script></html>"

#define BLYNK_SERVER_HARDWARE_PORT    8080

#define BLYNK_BOARD_TYPE      "ESP32_WFM"
#define NO_CONFIG             "nothing"

class BlynkWifi
  : public BlynkProtocol<BlynkArduinoClient>
{
    typedef BlynkProtocol<BlynkArduinoClient> Base;
  public:
    BlynkWifi(BlynkArduinoClient& transp)
      : Base(transp)
    {}

    void connectWiFi(const char* ssid, const char* pass)
    {
      BLYNK_LOG2(BLYNK_F("Con2:"), ssid);
      WiFi.mode(WIFI_STA);

      // New from Blynk_WM v1.0.5
      if (static_IP != IPAddress(0, 0, 0, 0))
      {
        BLYNK_LOG1(BLYNK_F("Use statIP"));
        WiFi.config(static_IP, static_GW, static_SN, static_DNS1, static_DNS2);
      }
      setHostname();

      if (pass && strlen(pass)) {
        WiFi.begin(ssid, pass);
      } else {
        WiFi.begin(ssid);
      }
      while (WiFi.status() != WL_CONNECTED) {
        BlynkDelay(500);
      }
      BLYNK_LOG1(BLYNK_F("Con2WiFi"));
      
      displayWiFiData();
    }

    void config(const char* auth,
                const char* domain = BLYNK_DEFAULT_DOMAIN,
                uint16_t    port   = BLYNK_DEFAULT_PORT)
    {
      Base::begin(auth);
      this->conn.begin(domain, port);
    }

    void config(const char* auth,
                IPAddress   ip,
                uint16_t    port = BLYNK_DEFAULT_PORT)
    {
      Base::begin(auth);
      this->conn.begin(ip, port);
    }

    void begin(const char* auth,
               const char* ssid,
               const char* pass,
               const char* domain = BLYNK_DEFAULT_DOMAIN,
               uint16_t    port   = BLYNK_DEFAULT_PORT)
    {
      connectWiFi(ssid, pass);
      config(auth, domain, port);
      while (this->connect() != true) {}
    }

    void begin(const char* auth,
               const char* ssid,
               const char* pass,
               IPAddress   ip,
               uint16_t    port   = BLYNK_DEFAULT_PORT)
    {
      connectWiFi(ssid, pass);
      config(auth, ip, port);
      while (this->connect() != true) {}
    }

#ifndef LED_BUILTIN
#define LED_BUILTIN       2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
#endif

    void begin(const char *iHostname = "")
    {
#define TIMEOUT_CONNECT_WIFI			30000

      //Turn OFF
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, LOW);

      WiFi.mode(WIFI_STA);

      if (iHostname[0] == 0)
      {
        String _hostname = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        _hostname.toUpperCase();

        getRFC952_hostname(_hostname.c_str());

      }
      else
      {
        // Prepare and store the hostname only not NULL
        getRFC952_hostname(iHostname);
      }

      BLYNK_LOG2(BLYNK_F("Hostname="), RFC952_hostname);

      if (getConfigData())
      {
        hadConfigData = true;

        Base::begin(BlynkESP32_WM_config.blynk_token);
        this->conn.begin(BlynkESP32_WM_config.blynk_server, BlynkESP32_WM_config.blynk_port);

        if (connectToWifi(TIMEOUT_CONNECT_WIFI))
        {
          BLYNK_LOG1(BLYNK_F("b:WOK.TryB"));

          int i = 0;
          while ( (i++ < 10) && !this->connect() )
          {
          }

          if  (this->connected())
          {
            BLYNK_LOG1(BLYNK_F("b:WBOK"));
          }
          else
          {
            BLYNK_LOG1(BLYNK_F("b:WOK,Bno"));
            // failed to connect to Blynk server, will start configuration mode
            startConfigurationMode();
          }
        }
        else
        {
          BLYNK_LOG1(BLYNK_F("b:FailW+B"));
          // failed to connect to Blynk server, will start configuration mode
          startConfigurationMode();
        }
      }
      else
      {
        BLYNK_LOG1(BLYNK_F("b:Nodat.Stay"));
        // failed to connect to Blynk server, will start configuration mode
        hadConfigData = false;
        startConfigurationMode();
      }
    }

#ifndef TIMEOUT_RECONNECT_WIFI
#define TIMEOUT_RECONNECT_WIFI   10000L
#else
    // Force range of user-defined TIMEOUT_RECONNECT_WIFI between 10-60s
#if (TIMEOUT_RECONNECT_WIFI < 10000L)
#warning TIMEOUT_RECONNECT_WIFI too low. Reseting to 10000
#undef TIMEOUT_RECONNECT_WIFI
#define TIMEOUT_RECONNECT_WIFI   10000L
#elif (TIMEOUT_RECONNECT_WIFI > 60000L)
#warning TIMEOUT_RECONNECT_WIFI too high. Reseting to 60000
#undef TIMEOUT_RECONNECT_WIFI
#define TIMEOUT_RECONNECT_WIFI   60000L
#endif
#endif

#ifndef RESET_IF_CONFIG_TIMEOUT
#define RESET_IF_CONFIG_TIMEOUT   true
#endif

#ifndef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET          10
#else
    // Force range of user-defined TIMES_BEFORE_RESET between 2-100
#if (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET < 2)
#warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too low. Reseting to 2
#undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET   2
#elif (CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET > 100)
#warning CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET too high. Reseting to 100
#undef CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET   100
#endif
#endif

    void run()
    {
      static int retryTimes = 0;

      // Lost connection in running. Give chance to reconfig.
      if ( WiFi.status() != WL_CONNECTED || !connected() )
      {
        // If configTimeout but user hasn't connected to configWeb => try to reconnect WiFi / Blynk.
        // But if user has connected to configWeb, stay there until done, then reset hardware
        if ( configuration_mode && ( configTimeout == 0 ||  millis() < configTimeout ) )
        {
          retryTimes = 0;

          if (server)
            server->handleClient();

          return;
        }
        else
        {
#if RESET_IF_CONFIG_TIMEOUT
          // If we're here but still in configuration_mode, permit running TIMES_BEFORE_RESET times before reset hardware
          // to permit user another chance to config.
          if ( configuration_mode && (configTimeout != 0) )
          {
            if (++retryTimes <= CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET)
            {
              BLYNK_LOG2(BLYNK_F("r:Wlost&TOut.ConW+B.Retry#"), retryTimes);
            }
            else
            {
              ESP.restart();
            }
          }
#endif

          // Not in config mode, try reconnecting before force to config mode
          if ( WiFi.status() != WL_CONNECTED )
          {
            BLYNK_LOG1(BLYNK_F("r:Wlost.ReconW+B"));
            if (connectToWifi(TIMEOUT_RECONNECT_WIFI))
            {
              // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
              digitalWrite(LED_BUILTIN, LOW);

              BLYNK_LOG1(BLYNK_F("r:WOK.TryB"));

              if (connect())
              {
                BLYNK_LOG1(BLYNK_F("r:W+BOK"));
              }
            }
          }
          else
          {
            BLYNK_LOG1(BLYNK_F("r:Blost.TryB"));
            if (connect())
            {
              // turn the LED_BUILTIN OFF to tell us we exit configuration mode.
              digitalWrite(LED_BUILTIN, LOW);

              BLYNK_LOG1(BLYNK_F("r:BOK"));
            }
          }

          //BLYNK_LOG1(BLYNK_F("run: Lost connection => configMode"));
          //startConfigurationMode();
        }
      }
      else if (configuration_mode)
      {
        configuration_mode = false;
        BLYNK_LOG1(BLYNK_F("r:gotW+Bback"));
        // Turn the LED_BUILTIN OFF when out of configuration mode. ESP32 LED_BUILDIN is correct polarity, LOW to turn OFF
        digitalWrite(LED_BUILTIN, LOW);
      }

      if (connected())
      {
        Base::run();
      }
    }

    void setHostname(void)
    {
      if (RFC952_hostname[0] != 0)
      {
        // See https://github.com/espressif/arduino-esp32/issues/2537
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(RFC952_hostname);
      }
    }

    void setConfigPortalIP(IPAddress portalIP = IPAddress(192, 168, 4, 1))
    {
      portal_apIP = portalIP;
    }

    void setConfigPortal(String ssid = "", String pass = "")
    {
      portal_ssid = ssid;
      portal_pass = pass;
    }

    void setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn = IPAddress(255, 255, 255, 0),
                              IPAddress dns_address_1 = IPAddress(0, 0, 0, 0),
                              IPAddress dns_address_2 = IPAddress(0, 0, 0, 0))
    {
      static_IP     = ip;
      static_GW     = gw;
      static_SN     = sn;

      // Default to local GW
      if (dns_address_1 == IPAddress(0, 0, 0, 0))
        static_DNS1   = gw;
      else
        static_DNS1   = dns_address_1;

      // Default to Google DNS (8, 8, 8, 8)
      if (dns_address_2 == IPAddress(0, 0, 0, 0))
        static_DNS2   = IPAddress(8, 8, 8, 8);
      else
        static_DNS2   = dns_address_2;
    }

    String getServerName()
    {
      if (!hadConfigData)
        getConfigData();

      return (String(BlynkESP32_WM_config.blynk_server));
    }

    String getToken()
    {
      if (!hadConfigData)
        getConfigData();

      return (String(BlynkESP32_WM_config.blynk_token));
    }

    String getBlynkBTToken(void)
    {
      if (!hadConfigData)
        getConfigData();

      return (String(BlynkESP32_WM_config.blynk_bt_tk));
    }

    String getBlynkBLEToken(void)
    {
      if (!hadConfigData)
        getConfigData();

      return (String(BlynkESP32_WM_config.blynk_ble_tk));
    }

    String getBoardName()
    {
      if (!hadConfigData)
        getConfigData();

      return (String(BlynkESP32_WM_config.board_name));
    }

    int getHWPort()
    {
      if (!hadConfigData)
        getConfigData();

      return (BlynkESP32_WM_config.blynk_port);
    }


    Blynk_WF_Configuration* getFullConfigData(Blynk_WF_Configuration *configData)
    {
      if (!hadConfigData)
        getConfigData();

      // Check if NULL pointer
      if (configData)
        memcpy(configData, &BlynkESP32_WM_config, sizeof(Blynk_WF_Configuration));

      return (configData);
    }

    void clearConfigData()
    {
      memset(&BlynkESP32_WM_config, 0, sizeof(BlynkESP32_WM_config));
      saveConfigData();
    }

  private:
    WebServer *server;
    bool configuration_mode = false;

    unsigned long configTimeout;
    bool hadConfigData = false;

    Blynk_WF_Configuration BlynkESP32_WM_config;

    // For Config Portal, from Blynk_WM v1.0.5
    IPAddress portal_apIP = IPAddress(192, 168, 4, 1);

    String portal_ssid = "";
    String portal_pass = "";

    // For static IP, from Blynk_WM v1.0.5
    IPAddress static_IP   = IPAddress(0, 0, 0, 0);
    IPAddress static_GW   = IPAddress(0, 0, 0, 0);
    IPAddress static_SN   = IPAddress(255, 255, 255, 0);
    IPAddress static_DNS1 = IPAddress(0, 0, 0, 0);
    IPAddress static_DNS2 = IPAddress(0, 0, 0, 0);

#define RFC952_HOSTNAME_MAXLEN      24
    char RFC952_hostname[RFC952_HOSTNAME_MAXLEN + 1];

    char* getRFC952_hostname(const char* iHostname)
    {
      memset(RFC952_hostname, 0, sizeof(RFC952_hostname));

      size_t len = ( RFC952_HOSTNAME_MAXLEN < strlen(iHostname) ) ? RFC952_HOSTNAME_MAXLEN : strlen(iHostname);

      size_t j = 0;

      for (size_t i = 0; i < len - 1; i++)
      {
        if ( isalnum(iHostname[i]) || iHostname[i] == '-' )
        {
          RFC952_hostname[j] = iHostname[i];
          j++;
        }
      }
      // no '-' as last char
      if ( isalnum(iHostname[len - 1]) || (iHostname[len - 1] != '-') )
        RFC952_hostname[j] = iHostname[len - 1];

      return RFC952_hostname;
    }

    void displayConfigData(void)
    {
      BLYNK_LOG6(BLYNK_F("Hdr="), BlynkESP32_WM_config.header, BLYNK_F(",SSID="), BlynkESP32_WM_config.wifi_ssid,
                 BLYNK_F(",PW="),     BlynkESP32_WM_config.wifi_pw);
      BLYNK_LOG6(BLYNK_F("Server="), BlynkESP32_WM_config.blynk_server, BLYNK_F(",Port="), BlynkESP32_WM_config.blynk_port,
                 BLYNK_F(",Token="),  BlynkESP32_WM_config.blynk_token);
      BLYNK_LOG4(BLYNK_F("BT-Token="), BlynkESP32_WM_config.blynk_bt_tk, BLYNK_F(",BLE-Token="), BlynkESP32_WM_config.blynk_ble_tk);
      BLYNK_LOG2(BLYNK_F("BrdName="), BlynkESP32_WM_config.board_name);
    }
    
    void displayWiFiData(void)
    {
      BLYNK_LOG6(BLYNK_F("IP="), WiFi.localIP().toString(), BLYNK_F(",GW="), WiFi.gatewayIP().toString(),
                   BLYNK_F(",SN="), WiFi.subnetMask().toString());
      BLYNK_LOG4(BLYNK_F("DNS1="), WiFi.dnsIP(0).toString(), BLYNK_F(",DNS2="), WiFi.dnsIP(1).toString());
    }

    int calcChecksum()
    {
      int checkSum = 0;
      for (uint16_t index = 0; index < (sizeof(BlynkESP32_WM_config) - sizeof(BlynkESP32_WM_config.checkSum)); index++)
      {
        checkSum += * ( ( (byte*) &BlynkESP32_WM_config ) + index);
      }

      return checkSum;
    }

#if USE_SPIFFS

#define  CONFIG_FILENAME         BLYNK_F("/wfm_config.dat")
#define  CONFIG_FILENAME_BACKUP  BLYNK_F("/wfm_config.bak")

    void loadConfigData(void)
    {
      File file = SPIFFS.open(CONFIG_FILENAME, "r");
      BLYNK_LOG1(BLYNK_F("LoadCfgFile "));

      if (!file)
      {
        BLYNK_LOG1(BLYNK_F("failed"));

        // Trying open redundant config file
        file = SPIFFS.open(CONFIG_FILENAME_BACKUP, "r");
        BLYNK_LOG1(BLYNK_F("Load BkUpCfgFile "));

        if (!file)
        {
          BLYNK_LOG1(BLYNK_F("failed"));
          return;
        }
      }

      file.readBytes((char *) &BlynkESP32_WM_config, sizeof(BlynkESP32_WM_config));

      BLYNK_LOG1(BLYNK_F("OK"));
      file.close();
    }

    void saveConfigData(void)
    {
      File file = SPIFFS.open(CONFIG_FILENAME, "w");

      int calChecksum = calcChecksum();
      BlynkESP32_WM_config.checkSum = calChecksum;
      BLYNK_LOG2(BLYNK_F("SaveCfgFile,CSum=0x"), String(calChecksum, HEX));

      if (file)
      {
        file.write((uint8_t*) &BlynkESP32_WM_config, sizeof(BlynkESP32_WM_config));
        file.close();
        BLYNK_LOG1(BLYNK_F("OK"));
      }
      else
      {
        BLYNK_LOG1(BLYNK_F("failed"));
      }

      // Trying open redundant Auth file
      file = SPIFFS.open(CONFIG_FILENAME_BACKUP, "w");
      BLYNK_LOG1(BLYNK_F("SaveBkUpCfgFile "));

      if (file)
      {
        file.write((uint8_t *) &BlynkESP32_WM_config, sizeof(BlynkESP32_WM_config));
        file.close();
        BLYNK_LOG1(BLYNK_F("OK"));
      }
      else
      {
        BLYNK_LOG1(BLYNK_F("failed"));
      }
    }

    // Return false if init new EEPROM or SPIFFS. No more need trying to connect. Go directly to config mode
    bool getConfigData()
    {
      if (!SPIFFS.begin())
      {
        BLYNK_LOG1(BLYNK_F("SPIFFS failed! Use EEPROM."));
        return false;
      }

      if ( SPIFFS.exists(CONFIG_FILENAME) || SPIFFS.exists(CONFIG_FILENAME_BACKUP) )
      {
        // if config file exists, load
        loadConfigData();
      }

      int calChecksum = calcChecksum();

      BLYNK_LOG4(BLYNK_F("CCSum=0x"), String(calChecksum, HEX),
                 BLYNK_F(",RCSum=0x"), String(BlynkESP32_WM_config.checkSum, HEX));

      //displayConfigData();

      if ( (strncmp(BlynkESP32_WM_config.header, BLYNK_BOARD_TYPE, strlen(BLYNK_BOARD_TYPE)) != 0) ||
           (calChecksum != BlynkESP32_WM_config.checkSum) )
      {
        memset(&BlynkESP32_WM_config, 0, sizeof(BlynkESP32_WM_config));

        BLYNK_LOG2(BLYNK_F("InitCfgFile,sz="), sizeof(BlynkESP32_WM_config));
        // doesn't have any configuration
        strcpy(BlynkESP32_WM_config.header,           BLYNK_BOARD_TYPE);
        strcpy(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG);
        strcpy(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG);
        strcpy(BlynkESP32_WM_config.blynk_server,     NO_CONFIG);
        BlynkESP32_WM_config.blynk_port = BLYNK_SERVER_HARDWARE_PORT;
        strcpy(BlynkESP32_WM_config.blynk_token,      NO_CONFIG);
        strcpy(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG);
        strcpy(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG);
        strcpy(BlynkESP32_WM_config.board_name,       NO_CONFIG);
        // Don't need
        BlynkESP32_WM_config.checkSum = 0;

        saveConfigData();

        return false;
      }
      else if ( !strncmp(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG, strlen(NO_CONFIG))   ||
                !strncmp(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_server,     NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_token,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG, strlen(NO_CONFIG) ) )
      {
        // If SSID, PW, Server,Token ="nothing", stay in config mode forever until having config Data.
        return false;
      }
      else
      {
        displayConfigData();
      }

      return true;
    }


#else

#ifndef EEPROM_SIZE
#define EEPROM_SIZE     512
#else
#if (EEPROM_SIZE > 2048)
#warning EEPROM_SIZE must be <= 2048. Reset to 2048
#undef EEPROM_SIZE
#define EEPROM_SIZE     2048
#endif
#if (EEPROM_SIZE < CONFIG_DATA_SIZE)
#warning EEPROM_SIZE must be > CONFIG_DATA_SIZE. Reset to 512
#undef EEPROM_SIZE
#define EEPROM_SIZE     512
#endif
#endif

#ifndef EEPROM_START
#define EEPROM_START     0
#else
#if (EEPROM_START + CONFIG_DATA_SIZE > EEPROM_SIZE)
#error EPROM_START + CONFIG_DATA_SIZE > EEPROM_SIZE. Please adjust.
#endif
#endif

    // Return false if init new EEPROM or SPIFFS. No more need trying to connect. Go directly to config mode
    bool getConfigData()
    {
      EEPROM.begin(EEPROM_SIZE);
      EEPROM.get(EEPROM_START, BlynkESP32_WM_config);

      int calChecksum = calcChecksum();

      BLYNK_LOG4(BLYNK_F("CCSum=0x"), String(calChecksum, HEX),
                 BLYNK_F(",RCSum=0x"), String(BlynkESP32_WM_config.checkSum, HEX));

      if ( (strncmp(BlynkESP32_WM_config.header, BLYNK_BOARD_TYPE, strlen(BLYNK_BOARD_TYPE)) != 0) ||
           (calChecksum != BlynkESP32_WM_config.checkSum) )
      {
        memset(&BlynkESP32_WM_config, 0, sizeof(BlynkESP32_WM_config));

        BLYNK_LOG2(BLYNK_F("InitEEPROM,sz="), EEPROM_SIZE /*EEPROM.length()*/);
        // doesn't have any configuration
        strcpy(BlynkESP32_WM_config.header,           BLYNK_BOARD_TYPE);
        strcpy(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG);
        strcpy(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG);
        strcpy(BlynkESP32_WM_config.blynk_server,     NO_CONFIG);
        BlynkESP32_WM_config.blynk_port = BLYNK_SERVER_HARDWARE_PORT;
        strcpy(BlynkESP32_WM_config.blynk_token,      NO_CONFIG);
        strcpy(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG);
        strcpy(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG);
        strcpy(BlynkESP32_WM_config.board_name,       NO_CONFIG);
        // Don't need
        BlynkESP32_WM_config.checkSum = 0;

        EEPROM.put(EEPROM_START, BlynkESP32_WM_config);
        EEPROM.commit();

        return false;
      }
      else if ( !strncmp(BlynkESP32_WM_config.wifi_ssid,        NO_CONFIG, strlen(NO_CONFIG))   ||
                !strncmp(BlynkESP32_WM_config.wifi_pw,          NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_server,     NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_token,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_bt_tk,      NO_CONFIG, strlen(NO_CONFIG) )  ||
                !strncmp(BlynkESP32_WM_config.blynk_ble_tk,     NO_CONFIG, strlen(NO_CONFIG) ) )
      {
        // If SSID, PW, Server,Token ="nothing", stay in config mode forever until having config Data.
        return false;
      }
      else
      {
        displayConfigData();
      }
      return true;
    }

    void saveConfigData()
    {
      int calChecksum = calcChecksum();
      BlynkESP32_WM_config.checkSum = calChecksum;
      BLYNK_LOG4(BLYNK_F("SaveEEPROM,sz="), EEPROM_SIZE /*EEPROM.length()*/,
                 BLYNK_F(",CSum=0x"), String(calChecksum, HEX));

      EEPROM.put(EEPROM_START, BlynkESP32_WM_config);
      EEPROM.commit();
    }

#endif

    boolean connectToWifi(int timeout)
    {
      int sleep_time = 250;

      WiFi.mode(WIFI_STA);
      setHostname();

      // New from Blynk_WM v1.0.5
      if (static_IP != IPAddress(0, 0, 0, 0))
      {
        BLYNK_LOG1(BLYNK_F("Use statIP"));
        WiFi.config(static_IP, static_GW, static_SN, static_DNS1, static_DNS2);
      }

      BLYNK_LOG1(BLYNK_F("con2WF:start"));

      if (BlynkESP32_WM_config.wifi_pw && strlen(BlynkESP32_WM_config.wifi_pw))
      {
        WiFi.begin(BlynkESP32_WM_config.wifi_ssid, BlynkESP32_WM_config.wifi_pw);
      }
      else
      {
        WiFi.begin(BlynkESP32_WM_config.wifi_ssid);
      }

      while (WiFi.status() != WL_CONNECTED && 0 < timeout)
      {
        delay(sleep_time);
        timeout -= sleep_time;
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        BLYNK_LOG1(BLYNK_F("con2WF:conOK"));
        displayWiFiData();
      }
      else
      {
        BLYNK_LOG1(BLYNK_F("con2WF:conFailed"));
      }

      return WiFi.status() == WL_CONNECTED;
    }


    void handleRequest()
    {
      if (server)
      {
        String key = server->arg("key");
        String value = server->arg("value");

        static int number_items_Updated = 0;

        if (key == "" && value == "")
        {
          String result = root_html_template;

          BLYNK_LOG1(BLYNK_F("h:repl"));

          // Reset configTimeout to stay here until finished.
          configTimeout = 0;

          result.replace("[[id]]",   BlynkESP32_WM_config.wifi_ssid);
          result.replace("[[pw]]",   BlynkESP32_WM_config.wifi_pw);
          result.replace("[[sv]]",   BlynkESP32_WM_config.blynk_server);
          result.replace("[[pt]]",   String(BlynkESP32_WM_config.blynk_port));
          result.replace("[[tk]]",   BlynkESP32_WM_config.blynk_token);
          result.replace("[[tk1]]",  BlynkESP32_WM_config.blynk_bt_tk);
          result.replace("[[tk2]]",  BlynkESP32_WM_config.blynk_ble_tk);
          result.replace("[[nm]]",   BlynkESP32_WM_config.board_name);

          server->send(200, "text/html", result);

          return;
        }

        if (number_items_Updated == 0)
        {
          memset(&BlynkESP32_WM_config, 0, sizeof(BlynkESP32_WM_config));
          strcpy(BlynkESP32_WM_config.header, BLYNK_BOARD_TYPE);
        }

        if (key == "id")
        {
          number_items_Updated++;
          if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.wifi_ssid) - 1)
            strcpy(BlynkESP32_WM_config.wifi_ssid, value.c_str());
          else
            strncpy(BlynkESP32_WM_config.wifi_ssid, value.c_str(), sizeof(BlynkESP32_WM_config.wifi_ssid) - 1);
        }
        else if (key == "pw")
        {
          number_items_Updated++;
          if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.wifi_pw) - 1)
            strcpy(BlynkESP32_WM_config.wifi_pw, value.c_str());
          else
            strncpy(BlynkESP32_WM_config.wifi_pw, value.c_str(), sizeof(BlynkESP32_WM_config.wifi_pw) - 1);
        }

        else if (key == "sv")
        {
          number_items_Updated++;
          if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_server) - 1)
            strcpy(BlynkESP32_WM_config.blynk_server, value.c_str());
          else
            strncpy(BlynkESP32_WM_config.blynk_server, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_server) - 1);
        }
        else if (key == "pt")
        {
          number_items_Updated++;
          BlynkESP32_WM_config.blynk_port = value.toInt();
        }
        else if (key == "tk")
        {
          number_items_Updated++;
          if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_token) - 1)
            strcpy(BlynkESP32_WM_config.blynk_token, value.c_str());
          else
            strncpy(BlynkESP32_WM_config.blynk_token, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_token) - 1);
        }
        else if (key == "tk1")
        {
          number_items_Updated++;
          if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_bt_tk) - 1)
            strcpy(BlynkESP32_WM_config.blynk_bt_tk, value.c_str());
          else
            strncpy(BlynkESP32_WM_config.blynk_bt_tk, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_bt_tk) - 1);
        }
        else if (key == "tk2")
        {
          number_items_Updated++;
          if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.blynk_ble_tk) - 1)
            strcpy(BlynkESP32_WM_config.blynk_ble_tk, value.c_str());
          else
            strncpy(BlynkESP32_WM_config.blynk_ble_tk, value.c_str(), sizeof(BlynkESP32_WM_config.blynk_ble_tk) - 1);
        }
        else if (key == "nm")
        {
          number_items_Updated++;
          if (strlen(value.c_str()) < sizeof(BlynkESP32_WM_config.board_name) - 1)
            strcpy(BlynkESP32_WM_config.board_name, value.c_str());
          else
            strncpy(BlynkESP32_WM_config.board_name, value.c_str(), sizeof(BlynkESP32_WM_config.board_name) - 1);
        }

        server->send(200, "text/html", "OK");

        if (number_items_Updated == NUM_CONFIGURABLE_ITEMS)
        {
#if USE_SPIFFS
          BLYNK_LOG2(BLYNK_F("h:UpdSPIFFS "), CONFIG_FILENAME);
#else
          BLYNK_LOG1(BLYNK_F("h:UpdEEPROM"));
#endif

          saveConfigData();

          BLYNK_LOG1(BLYNK_F("h:Rst"));

          // Delay then reset the ESP8266 after save data
          delay(1000);
          ESP.restart();
        }
      }    // if (server)
    }

    void startConfigurationMode()
    {
#define CONFIG_TIMEOUT			60000L

      // turn the LED_BUILTIN ON to tell us we are in configuration mode.
      digitalWrite(LED_BUILTIN, HIGH);

      if ( (portal_ssid == "") || portal_pass == "" )
      {
        String chipID = String(ESP_getChipId(), HEX);
        chipID.toUpperCase();

        portal_ssid = "ESP_" + chipID;

        portal_pass = "MyESP_" + chipID;
      }

      BLYNK_LOG6(BLYNK_F("stConf:SSID="), portal_ssid, BLYNK_F(",PW="), portal_pass,
                 BLYNK_F(",IP="), portal_apIP.toString());

      WiFi.mode(WIFI_AP);
      WiFi.softAP(portal_ssid.c_str(), portal_pass.c_str());

      delay(100); // ref: https://github.com/espressif/arduino-esp32/issues/985#issuecomment-359157428
      WiFi.softAPConfig(portal_apIP, portal_apIP, IPAddress(255, 255, 255, 0));

      if (!server)
        server = new WebServer;

      //See https://stackoverflow.com/questions/39803135/c-unresolved-overloaded-function-type?rq=1
      if (server)
      {
        server->on("/", [this]() { handleRequest(); });
        server->begin();
      }

      // If there is no saved config Data, stay in config mode forever until having config Data.
      if (hadConfigData)
        configTimeout = millis() + CONFIG_TIMEOUT;
      else
        configTimeout = 0;

      configuration_mode = true;
    }
};

static WiFiClient _blynkWifiClient;
static BlynkArduinoClient _blynkTransport(_blynkWifiClient);

// KH
BlynkWifi Blynk_WF(_blynkTransport);

#if defined(Blynk)
#undef Blynk
#define Blynk Blynk_WF
#endif
//

#include <BlynkWidgets.h>

#endif
