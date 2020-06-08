#ifndef __USER_WEBSERVER_H__
#define __USER_WEBSERVER_H__

#define SERVER_PORT 80



#define URL_MAX_LENGTH 16
#define URL_GET_DAT_MAX_LENGTH 255
#define URL_POST_DAT_MAX_LENGTH 255
#define URLSize 10

typedef enum Result_Resp {
    RespFail = 0,
    RespSuc,
} Result_Resp;

typedef enum ProtocolType {
	ERROR=-1,
    GET = 0,
    POST,//暂时仅实现GET POST 其他不支持
	OPTIONS,
	HEAD,
	PUT,
	DELETE,
	TRACE,
	CONNECT
} ProtocolType;

typedef enum _ParmType {
    SWITCH_STATUS = 0,
    INFOMATION,
    WIFI,
    SCAN,
	REBOOT,
    DEEP_SLEEP,
    LIGHT_STATUS,
    CONNECT_STATUS,
    USER_BIN
} ParmType;

typedef struct URL_Frame {
    enum ProtocolType Type;
    char pUri[URL_MAX_LENGTH];
    char pGetdat[URL_GET_DAT_MAX_LENGTH];
    char pPostdat[URL_POST_DAT_MAX_LENGTH];


} URL_Frame;

typedef struct _rst_parm {
    ParmType parmtype;
    struct espconn *pespconn;
} rst_parm;


struct http_uri_call {
	/** URI of the WSGI */
	const char *uri;
	/** Indicator for HTTP headers to be sent in the response*/
//	int hdr_fields;
	/** Flag indicating if exact match of the URI is required or not */
	//int http_flags;
	/** HTTP GET or HEAD Handler */
	int (*get_handler) (void *arg, URL_Frame *purl_frame);
	/** HTTP POST Handler */
	int (*set_handler) (void *arg, URL_Frame *purl_frame);
	/** HTTP PUT Handler */
	int (*put_handler) (void *arg, URL_Frame *purl_frame);
	/** HTTP DELETE Handler */
	int (*delete_handler) (void *arg, URL_Frame *purl_frame);
};

void user_webserver_init(uint32 port);

#endif
