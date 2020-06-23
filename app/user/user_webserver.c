#include "stdlib.h"

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "espconn.h"

#include "user_webserver.h"
#include "user_wifi.h"

extern const unsigned char wififail[0xAC9];

extern const unsigned char wifisetting[3372];

extern const unsigned char wifisuccess[0x9BC];

LOCAL void ICACHE_FLASH_ATTR data_send(void *arg, bool responseOK, char *psend);

LOCAL int ICACHE_FLASH_ATTR char2hex(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xA;
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xa;

	return -1;
}
/*
 * html_decode html字符反转义,将类似%40转换为对应特殊字符@
 * old: 需要反转义的字符串指针
 * new: 反转义后的字符串指针
 */
LOCAL void ICACHE_FLASH_ATTR html_decode(char *old, char *new) {
	int i, c1, c2;
	char *old_p;
	char *new_p;

	if (old == NULL || new == NULL || *old == '\0')
		return;

	old_p = old;
	new_p = new;

	os_printf("html_decode:%s\n\t ", old);
	while (1) {
//		os_printf("%c  ", *old_p);
		if (*old_p == 0) {
			*new_p='\0';
			break;
		}
		if (*old_p == '%') {
			old_p++;
			c1 = char2hex(*old_p);
			old_p++;
			c2 = char2hex(*old_p);

			if (c1 == -1 || c2 == -1) {
				*new_p = '\0';
				os_printf("c1:%x\tc2:%x\n\t ", c1,c2);
				return;
			}
			*new_p = ((c1 << 4) | c2);
			new_p++;
		} else {
			*new_p = *old_p;
			new_p++;
		}
		old_p++;
	}os_printf("\n", old);

}

void ICACHE_FLASH_ATTR web_send_wifisetting_page(void *arg, URL_Frame *purl_frame) {
	struct espconn *ptrespconn = arg;
	data_send(ptrespconn, true, wifisetting);
}

void ICACHE_FLASH_ATTR web_send_result_page(void *arg, URL_Frame *purl_frame) {
	struct espconn *ptrespconn = arg;

	char *str = NULL;
	uint8 length = 0;
	char *pbuffer = NULL;
	char *precv = NULL;
	char *pbufer = NULL;
	char ssid[32] = { 0 };
	char password[64] = { 0 };
	char ssid_encoded[32] = { 0 };
	char password_encoded[64] = { 0 };
	os_memset(ssid, 0, 32);
	os_memset(password, 0, 64);
	os_memset(ssid_encoded, 0, 32);
	os_memset(password_encoded, 0, 64);

	struct station_config stationConf;
	if (purl_frame == NULL) {
		goto Error;
	}
	precv = purl_frame->pPostdat;
	//获取SSID
	pbufer = (char *) os_strstr(precv, "SSID=");
	if (pbufer == NULL) {
		goto Error;
	}
	pbufer += 5;
	pbuffer = (char *) os_strstr(pbufer, "&");
	if (pbuffer == NULL) {
		length = precv + os_strlen(precv) - pbufer;
	} else {
		length = pbuffer - pbufer;
	}
	if (length > 31)
		goto Error;

	os_memcpy(ssid_encoded, pbufer, length);
	html_decode(ssid_encoded,ssid);
	os_printf("ssid_encoded:%s\n", ssid_encoded);
	os_printf("ssid:%s\n", ssid);

	//获取PASSWORD
	pbufer = (char *) os_strstr(precv, "PASS=");
	if (pbufer != NULL) {

		pbufer += 5;
		pbuffer = (char *) os_strstr(pbufer, "&");
		if (pbuffer == NULL) {
			length = precv + os_strlen(precv) - pbufer;
		} else {
			length = pbuffer - pbufer;
		}
		if (length > 63)
			goto Error;

		os_memcpy(password_encoded, pbufer, length);
		html_decode(password_encoded,password);
	}
	os_printf("password_encoded:%s\n", password_encoded);
	os_printf("password:%s\n", password);

	data_send(ptrespconn, true, wifisuccess);

	//连接wifi
	user_wifi_set(ssid, password);
	return;
	Error: data_send(ptrespconn, true, wififail);
}

struct http_uri_call g_app_handlers[] = { { "/", web_send_wifisetting_page, NULL, NULL, NULL }, { "/result.htm", NULL, web_send_result_page, NULL,
NULL }, { "/setting.htm", web_send_wifisetting_page, NULL, NULL, NULL }, };

/******************************************************************************
 * FunctionName : parse_url
 * Description  : parse the received data from the server
 * Parameters   : precv -- the received data
 *                purl_frame -- the result of parsing the url
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
parse_url(char *precv, URL_Frame *purl_frame) {
	char *str = NULL;
	uint8 length = 0;
	char *pbuffer = NULL;
	char *pbufer = NULL;

	if (purl_frame == NULL || precv == NULL) {
		return;
	}

	pbuffer = (char *) os_strstr(precv, "Host:");

	if (pbuffer != NULL) {
		length = pbuffer - precv;
		pbufer = (char *) os_zalloc(length + 1);
		pbuffer = pbufer;
		os_memcpy(pbuffer, precv, length);		//pbuffer 为第一行内容
		os_memset(purl_frame, 0, sizeof(struct URL_Frame));

		if (os_strncmp(pbuffer, "GET ", 4) == 0) {
			purl_frame->Type = GET;
			pbuffer += 4;
		} else if (os_strncmp(pbuffer, "POST ", 5) == 0) {
			purl_frame->Type = POST;
			pbuffer += 5;
		}

		//pbuffer++;
		str = (char *) os_strstr(pbuffer, "?");
		if (str != NULL) {
			length = str - pbuffer;
			os_memcpy(purl_frame->pUri, pbuffer, length);		//获取uri

			str = (char *) os_strstr(pbuffer, " HTTP");
			if (str != NULL) {
				pbuffer += length + 1;
				length = str - pbuffer;
				os_memcpy(purl_frame->pGetdat, pbuffer, length);		//获取uri中请求数据
			} else {
				purl_frame->Type = ERROR;
				return;
			}
		} else {
			str = (char *) os_strstr(pbuffer, " HTTP");

			if (str != NULL) {
				length = str - pbuffer;
				os_memcpy(purl_frame->pUri, pbuffer, length);		//获取uri,uri中无请求数据
			} else {
				purl_frame->Type = ERROR;
				return;
			}
		}

		str = (char *) os_strstr(precv, "\r\n\r\n");

		if (str != NULL) {
			str += 4;
			length = os_strlen(precv) - (str - precv);
			os_memcpy(purl_frame->pPostdat, str, length);
		}

		os_free(pbufer);
	} else {
		return;
	}
}

LOCAL char *precvbuffer;
static uint32 dat_sumlength = 0;
LOCAL bool ICACHE_FLASH_ATTR
save_data(char *precv, uint16 length) {
	bool flag = false;
	char length_buf[10] = { 0 };
	char *ptemp = NULL;
	char *pdata = NULL;
	uint16 headlength = 0;
	static uint32 totallength = 0;

	ptemp = (char *) os_strstr(precv, "\r\n\r\n");

	if (ptemp != NULL) {
		length -= ptemp - precv;
		length -= 4;
		totallength += length;
		headlength = ptemp - precv + 4;
		pdata = (char *) os_strstr(precv, "Content-Length: ");

		if (pdata != NULL) {
			pdata += 16;
			precvbuffer = (char *) os_strstr(pdata, "\r\n");

			if (precvbuffer != NULL) {
				os_memcpy(length_buf, pdata, precvbuffer - pdata);
				dat_sumlength = atoi(length_buf);
			}
		} else {
			if (totallength != 0x00) {
				totallength = 0;
				dat_sumlength = 0;
				return false;
			}
		}
		if ((dat_sumlength + headlength) >= 1024) {
			precvbuffer = (char *) os_zalloc(headlength + 1);
			os_memcpy(precvbuffer, precv, headlength + 1);
		} else {
			precvbuffer = (char *) os_zalloc(dat_sumlength + headlength + 1);
			os_memcpy(precvbuffer, precv, os_strlen(precv));
		}
	} else {
		if (precvbuffer != NULL) {
			totallength += length;
			os_memcpy(precvbuffer + os_strlen(precvbuffer), precv, length);
		} else {
			totallength = 0;
			dat_sumlength = 0;
			return false;
		}
	}

	if (totallength == dat_sumlength) {
		totallength = 0;
		dat_sumlength = 0;
		return true;
	} else {
		return false;
	}
}

LOCAL bool ICACHE_FLASH_ATTR
check_data(char *precv, uint16 length) {
	//bool flag = true;
	char length_buf[10] = { 0 };
	char *ptemp = NULL;
	char *pdata = NULL;
	char *tmp_precvbuffer;
	uint16 tmp_length = length;
	uint32 tmp_totallength = 0;

	ptemp = (char *) os_strstr(precv, "\r\n\r\n");

	if (ptemp != NULL) {
		tmp_length -= ptemp - precv;
		tmp_length -= 4;
		tmp_totallength += tmp_length;

		pdata = (char *) os_strstr(precv, "Content-Length: ");

		if (pdata != NULL) {
			pdata += 16;
			tmp_precvbuffer = (char *) os_strstr(pdata, "\r\n");

			if (tmp_precvbuffer != NULL) {
				os_memcpy(length_buf, pdata, tmp_precvbuffer - pdata);
				dat_sumlength = atoi(length_buf);
				os_printf("A_dat:%u,tot:%u,lenght:%u\n", dat_sumlength, tmp_totallength, tmp_length);
				if (dat_sumlength != tmp_totallength) {
					return false;
				}
			}
		}
	}
	return true;
}

/******************************************************************************
 * FunctionName : data_send
 * Description  : processing the data as http format and send to the client or server
 * Parameters   : arg -- argument to set for client or server
 *                responseOK -- true or false
 *                psend -- The send data
 * Returns      :
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
data_send(void *arg, bool responseOK, char *psend) {
	uint16 length = 0;
	char *pbuf = NULL;
	char httphead[256];
	struct espconn *ptrespconn = arg;
	os_memset(httphead, 0, 256);

	if (responseOK) {
		os_sprintf(httphead, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nServer: zS7\r\n", psend ? os_strlen(psend) : 0);

		if (psend) {
			os_sprintf(httphead + os_strlen(httphead),
					"Content-Type: text/html\r\nExpires: Fri, 10 Apr 2008 14:00:00 GMT\r\nPragma: no-cache\r\n\r\n");
			length = os_strlen(httphead) + os_strlen(psend);
			pbuf = (char *) os_zalloc(length + 1);
			os_memcpy(pbuf, httphead, os_strlen(httphead));
			os_memcpy(pbuf + os_strlen(httphead), psend, os_strlen(psend));
		} else {
			os_sprintf(httphead + os_strlen(httphead), "\n");
			length = os_strlen(httphead);
		}
	} else {
		os_sprintf(httphead, "HTTP/1.0 400 BadRequest\r\n\
Content-Length: 0\r\nServer: zS7\r\n\n");
		length = os_strlen(httphead);
	}

	if (psend) {
		espconn_sent(ptrespconn, pbuf, length);
	} else {
		espconn_sent(ptrespconn, httphead, length);
	}

	if (pbuf) {
		os_free(pbuf);
		pbuf = NULL;
	}
}
/******************************************************************************
 * FunctionName : response_send
 * Description  : processing the send result
 * Parameters   : arg -- argument to set for client or server
 *                responseOK --  true or false
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
response_send(void *arg, bool responseOK) {
	struct espconn *ptrespconn = arg;

	data_send(ptrespconn, responseOK, NULL);
}

/******************************************************************************
 * FunctionName : webserver_recv
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
webserver_recv(void *arg, char *pusrdata, unsigned short length) {
	URL_Frame *pURL_Frame = NULL;
	char *pParseBuffer = NULL;
	bool parse_flag = false;
	char i;
	struct espconn *ptrespconn = arg;

//	os_printf("len:%u\n", length);
	if (check_data(pusrdata, length) == false) {
//		os_printf("goto\n");
		goto _temp_exit;
	}

	parse_flag = save_data(pusrdata, length);
	if (parse_flag == false) {
		response_send(ptrespconn, false);
	}

	pURL_Frame = (URL_Frame *) os_zalloc(sizeof(URL_Frame));
	parse_url(pusrdata, pURL_Frame);
	os_printf("Type:%d\n", pURL_Frame->Type);
	os_printf("pSelect:%s\n", pURL_Frame->pUri);
	os_printf("pGetdat:%s\n", pURL_Frame->pGetdat);
	os_printf("pPostdat:%s\n", pURL_Frame->pPostdat);

	for (i = 0; i < sizeof(g_app_handlers) / sizeof(struct http_uri_call); i++) {

		if (os_strncmp(g_app_handlers[i].uri, pURL_Frame->pUri,		//os_strlen(pURL_Frame->pUri)
				((os_strlen(pURL_Frame->pUri) > os_strlen(g_app_handlers[i].uri)) ? os_strlen(pURL_Frame->pUri) : os_strlen(g_app_handlers[i].uri)))
				== 0) {
			switch (pURL_Frame->Type) {
			case GET:
				if (g_app_handlers[i].get_handler != NULL)
					g_app_handlers[i].get_handler(ptrespconn, pURL_Frame);
				break;
			case POST:
				if (g_app_handlers[i].set_handler != NULL)
					g_app_handlers[i].set_handler(ptrespconn, pURL_Frame);
				break;

			}
			break;
		}
	}
	if (i >= sizeof(g_app_handlers) / sizeof(struct http_uri_call)) {
//		os_printf("no http:%s\n", pURL_Frame->pUri);
		data_send(ptrespconn, false, NULL);
	}

	if (precvbuffer != NULL) {
		os_free(precvbuffer);
		precvbuffer = NULL;
	}
	os_free(pURL_Frame);
	pURL_Frame = NULL;
	_temp_exit: ;

}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL ICACHE_FLASH_ATTR
void webserver_recon(void *arg, sint8 err) {
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d err %d reconnect\n", pesp_conn->proto.tcp->remote_ip[0], pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2], pesp_conn->proto.tcp->remote_ip[3], pesp_conn->proto.tcp->remote_port, err);
}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL ICACHE_FLASH_ATTR
void webserver_discon(void *arg) {
	struct espconn *pesp_conn = arg;

	os_printf("webserver's %d.%d.%d.%d:%d disconnect\n", pesp_conn->proto.tcp->remote_ip[0], pesp_conn->proto.tcp->remote_ip[1],
			pesp_conn->proto.tcp->remote_ip[2], pesp_conn->proto.tcp->remote_ip[3], pesp_conn->proto.tcp->remote_port);
}

/******************************************************************************
 * FunctionName : user_accept_listen
 * Description  : server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
webserver_listen(void *arg) {
	struct espconn *pesp_conn = arg;

	espconn_regist_recvcb(pesp_conn, webserver_recv);
	espconn_regist_reconcb(pesp_conn, webserver_recon);
	espconn_regist_disconcb(pesp_conn, webserver_discon);
}

/******************************************************************************
 * FunctionName : user_webserver_init
 * Description  : parameter initialize as a server
 * Parameters   : port -- server port
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_webserver_init(uint32 port) {
	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, webserver_listen);

	espconn_accept(&esp_conn);

}
