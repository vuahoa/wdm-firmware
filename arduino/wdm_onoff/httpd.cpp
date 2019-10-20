#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "esp8266_mlib.h"
#include "httpd.h"


#define DB      Serial.printf
#ifndef DB
  #define DB
#endif

///////////////////////////////////////LOCAL CONSTANTS/////////////////////////////////////////////

///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
/* HTTP server */
WiFiServer server(80);

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
void httpd::init()
{
	server.begin();	
}

/**
 * Implement HTTP server to get configuration from HTTP client.
 * Return 1 if having new configuration, 0 if not.
*/
uint32_t httpd::get_config(ROM_SETTINGS_t *settings)
{
    WiFiClient client = server.available();
	String req = "";

    // wait for a client (web browser) to connect
    if (client) {
        DB("\r\n[Client connected]");
        while (client.connected())
        {
            // read line by line what the client (web browser) is requesting
            if (client.available())
            {
                char c = client.read();
                req += c;
                if (c == '\n') {                    
                    if (req.indexOf("GET /cfg?") != -1) {
                        char sbuf[64];
                        uint32_t val_sz = 0;
                        uint32_t param_cnt = 0;
    
                        val_sz = get_param(req.c_str(), "ssid=", sbuf, 64);
                        if ((val_sz > 0) && (val_sz < CFG_SSID_SZ)) {
                            parse_percent(sbuf);
                            sprintf(settings->ssid, sbuf, val_sz);
                            param_cnt++;
                        }
                        val_sz = get_param(req.c_str(), "password=", sbuf, 64);
                        if ((val_sz >= 8) && (val_sz < CFG_PASSWORD_SZ)) {
                            parse_percent(sbuf);
                            sprintf(settings->password, sbuf, val_sz);
                            param_cnt++;
                        }
                        val_sz = get_param(req.c_str(), "server=", sbuf, 64);
                        if ((val_sz > 0) && (val_sz < CFG_SERVER_SZ)) {
                            parse_percent(sbuf);
                            sprintf(settings->server_addr, sbuf, val_sz);
                            param_cnt++;
                        }
						val_sz = get_param(req.c_str(), "port=", sbuf, 64);
                        if ((val_sz > 0) && (val_sz < 10)) {
                            settings->server_port = atoi(sbuf);
                            param_cnt++;
                        }
                        val_sz = get_param(req.c_str(), "security=", sbuf, 64);
                        if ((val_sz > 0) && (val_sz < CFG_SECURITY_SZ)) {
                            parse_percent(sbuf);
                            sprintf(settings->security, sbuf, val_sz);
                            param_cnt++;
                        }                        
						val_sz = get_param(req.c_str(), "tz=", sbuf, 64);
                        if ((val_sz > 0) && (val_sz < 10)) {
                            settings->timezone = atoi(sbuf);
                            param_cnt++;
                        }
						
                        DB(" -> param_cnt=%d", param_cnt);
    
                        // Send result to client:
                        client.print(htmlCfg(param_cnt >= 5));
                        if (param_cnt >= 5) {
                            delay(1000);
                            return 1;
                        }
                        break;
                    } else {
                        client.print(htmlDefault());
                        delay(100);         
                        break;
                    }
                }
            }
        }
        req = "";
    }
    return 0;
}

// prepare a web page to be send to a client (web browser)
String httpd::htmlDefault()
{
    String htmlPage =
    String("HTTP/1.1 200 OK\r\n") +
            "Content-Type: text/html\r\n" +
            "Connection: close\r\n" +  // the connection will be closed after completion of the response
            "\r\n" +
            "MAC address: " + String(WiFi.macAddress()) + "\r\n" +
            "Format: /cfg?ssid=p1&password=p2&server=p3&security=p4" + "\r\n";
    return htmlPage;
}

String httpd::htmlCfg(bool result)
{
    String htmlPage =
     String("HTTP/1.1 200 nOt Found\r\n") +
            "Content-Type: text/html\r\n" +
            "Connection: close\r\n" +  // the connection will be closed after completion of the response
            "\r\n" +
            "<!DOCTYPE HTML>" +
            "<html>" +
            "Config Result: ";

    if (result) {
      htmlPage += "SUCCESS";   
    } else {
      htmlPage += "ERROR";
    }
    htmlPage += String("</html>\r\n");
    return htmlPage;
}

/** @brief replace the '%xx' by a ASCII character.
    @param *ptr the string to be parsed.
*/
void httpd::parse_percent(char *p_str)
{
    char x = 0;
    char *p2 = NULL;
    char *ptr = p_str;
    int32_t h = -1, l = -1, sz = 0;
    #ifdef _DB_PARSE_
        printf("\r\nParse%%: INPUT=[%s] ", p_str);
    #endif

    sz = strlen(p_str);
    while (*ptr != 0) {
        if (*ptr == '%') {
            h = *(ptr + 1);
            l = *(ptr + 2);
            if ((h >= 'A') && (h <= 'F')) {
                h = h - 'A' + 10;
            } else if ((h >= '0') && (h <= '9')) {
                h = h - '0';
            } else {
                h = -1;
            }
            if ((l >= 'A') && (l <= 'F')) {
                l = l - 'A' + 10;
            } else if ((l >= '0') && (l <= '9')) {
                l = l - '0';
            } else {
                l = -1;
            }
            if ((h >= 0) && (l >= 0)) {
                x = (h << 4) | l;

            #ifdef _DB_PARSE_
                printf(" [X=%c(%02Xh)]", x, x);
            #endif

                *ptr = x;
                for (p2 = ptr + 1; p2 - p_str < sz - 2; p2++) {
                    *p2 = *(p2 + 2);
                }
                *p2 = 0;
                sz -= 2;
            }

        #ifdef _DB_PARSE_
            printf(" ->STR=[%s] ", p_str);
        #endif
        }
        ptr++;
    }

    #ifdef _DB_PARSE_
        printf(" ->OUTPUT=[%s] ", p_str);
    #endif
}

uint8_t httpd::get_param(const char *req, const char *pName, char *val, char val_sz)
{
    uint8_t ret = 0;
    const char *p = strstr(req, pName);

    DB("\r\n%s: pName=%s, val_sz=%u", __FUNCTION__, pName, val_sz); 
    if (p != NULL) {
        p += strlen(pName);
        const char *p2 = strchr(p, '&');
        if (p2 == NULL) {
            p2 = strchr(p, ' ');
        }
        if (p2 != NULL) {
            int sz = p2 - p;
            if (sz < val_sz) {
            memcpy(val, p, sz);
            val[sz] = 0;
            ret = sz;
            }  
        }
    }
    DB(" -> ret=%u, val=[%s]", ret, val);
    return ret;
}
