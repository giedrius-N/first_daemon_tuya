#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <argp.h>
#include <signal.h>

#include "cJSON.h"
#include "tuya_cacert.h"
#include "tuya_log.h"
#include "tuya_error_code.h"
#include "system_interface.h"
#include "mqtt_client_interface.h"
#include "tuyalink_core.h"

#include <syslog.h>

char args_doc[] = "DeviceID DeviceSecret ProductID";

struct argp_option options[] = {
    	{"prodID", 'p', "STRING", 0, "DeviceID", 0},
    	{"devId", 'd', "STRING", 0, "DeviceSecret", 0},
    	{"devSec", 's', "STRING", 0, "ProductID", 0},
    	{"daemon", 'D', 0, 0, "Daemonize the process", 0},
    	{0}
};


struct arguments {
    	char prodId[50];
    	char devId[50];
    	char devSec[50];
    	bool daemonize;
};

error_t parse_opt(int key, char *arg, struct argp_state *state) {
    	struct arguments *arguments = state->input;

    	switch (key) {
        	case 'p':
            		strncpy(arguments->prodId, arg, sizeof(arguments->prodId) - 1);
            		arguments->prodId[sizeof(arguments->prodId) - 1] = '\0';
            		break;
        	case 'd':
		    	strncpy(arguments->devId, arg, sizeof(arguments->devId) - 1);
		    	arguments->devId[sizeof(arguments->devId) - 1] = '\0';
		    	break;
        	case 's':
		    	strncpy(arguments->devSec, arg, sizeof(arguments->devSec) - 1);
		    	arguments->devSec[sizeof(arguments->devSec) - 1] = '\0';
		    	break;
            	case 'D':
		    	arguments->daemonize = true;
		    	break;
        	case ARGP_KEY_END:
		    	if (state->arg_num != 0) {
		    		syslog(LOG_ERR, "ERROR: Excessive arguments. Expected ARG1 ARG2 ARG3");
		        	argp_usage(state);
		    	}
		    	break;
        	default:
            		return ARGP_ERR_UNKNOWN;
    	}
    	return 0;
}

struct argp argp = {options, parse_opt, args_doc, NULL};

void daemonize() {
    	pid_t pid, sid;

    	pid = fork();
    	if (pid < 0) {
    		syslog(LOG_ERR, "ERROR: Forking process failed. Unable to create child process.");
        	exit(EXIT_FAILURE);
    	}
    	if (pid > 0) {
    		syslog(LOG_INFO, "Child process successfully created.");
        	exit(EXIT_SUCCESS);
    	}

    	umask(0);

    	sid = setsid();
    	if (sid < 0) {
    		syslog(LOG_ERR, "ERROR: Session creation failed. Unable to create new session.");
        	exit(EXIT_FAILURE);
    	}	

    	close(STDIN_FILENO);
    	close(STDOUT_FILENO);
    	close(STDERR_FILENO);
}


volatile sig_atomic_t g_signal_flag = 1;
void sig_handler()
{
	g_signal_flag = 0;
}


//************* Tuya SDK *****************//
char productId[50];      // = "gu4dt5nzdjwjbyon";
char deviceId[50];       // = "26f4410c90739d522evbjp";
char deviceSecret[50];   // = "CtYs2K3cule42h5s";
struct arguments arguments;
tuya_mqtt_context_t client_instance;
tuya_mqtt_context_t* client;
int ret = OPRT_OK;

void on_connected(tuya_mqtt_context_t* context)
{
    	TY_LOGI("on connected");
    	syslog(LOG_INFO, "Connected to Tuya.");
    	
    	syslog(LOG_INFO, "Sending property report with ACK: {\"daemon_test\":{\"value\":true,\"time\":1631708204231}}");
    	tuyalink_thing_property_report_with_ack(context, deviceId, "{\"daemon_test\":{\"value\":true,\"time\":1631708204231}}");	

	syslog(LOG_INFO, "Sending property report with ACK: {\"greeting\":{\"value\":'Hello world!!---','time':1631708204231}}");
    	tuyalink_thing_property_report_with_ack(context, deviceId, "{\"greeting\":{\"value\":'Hello world!!---',\"time\":1631708204231}}");  
}

void on_disconnect(tuya_mqtt_context_t* context)
{
    	TY_LOGI("on disconnect");
    	syslog(LOG_INFO, "Disconnected from Tuya.");
}

void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg)
{
    	TY_LOGI("on message id:%s, type:%d, code:%d", msg->msgid, msg->type, msg->code);
    	switch (msg->type) {
        	case THING_TYPE_DEVICE_SUB_BIND_RSP:
            	TY_LOGI("bind response:%s\r\n", msg->data_string);
            	break;

        	case THING_TYPE_DEVICE_TOPO_GET_RSP:
            	TY_LOGI("get topo response:%s\r\n", msg->data_string);
            	break;

        	default:
            	break;
    	}
    	printf("\r\n");
}

void termination_handler(int signum) {
    	syslog(LOG_INFO, "Program terminated by signal %d.", signum);
    	closelog();
    	tuya_mqtt_disconnect(client);
	ret = 0;
    	exit(EXIT_SUCCESS);
}
/*
void sig_handler(int signum) {
    	syslog(LOG_INFO, "Program terminated by user in the console.");
    	closelog();
    	tuya_mqtt_disconnect(client);
    	
    	exit(EXIT_SUCCESS);
}*/

int main(int argc, char** argv) {
	setlogmask(LOG_UPTO (LOG_INFO));

	openlog("First daemon program", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	
	syslog(LOG_INFO, "Program started by User %s", "studentas");
	
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	
	
	
	arguments.prodId[0] = '\0';
   	arguments.devId[0] = '\0';
    	arguments.devSec[0] = '\0';
	
    	if (argp_parse(&argp, argc, argv, 0, NULL, &arguments) != 0) {
	    	syslog(LOG_ERR, "ERROR: Failed to parse command-line arguments.");
        	closelog();
	    	
	    	exit(EXIT_FAILURE);
	}
    	
    	if (arguments.prodId[0] == '\0' || arguments.devId[0] == '\0' || arguments.devSec[0] == '\0') {
        	syslog(LOG_ERR, "ERROR: Insufficient arguments. Expected ARG1 ARG2 ARG3");
        	closelog();

        	exit(EXIT_FAILURE);
    	}
    	
	if (arguments.daemonize) {
		daemonize();
		syslog(LOG_INFO, "Daemonization complete.");
	};
	
	strncpy(productId, arguments.prodId, 50);
	strncpy(deviceId, arguments.devId, 50);
	strncpy(deviceSecret, arguments.devSec, 50);

    	//int ret = OPRT_OK;

    	//tuya_mqtt_context_t* client = &client_instance;
	client = &client_instance;
	
    	ret = tuya_mqtt_init(client, &(const tuya_mqtt_config_t) {
        	.host = "m1.tuyacn.com",
        	.port = 8883,
        	.cacert = tuya_cacert_pem,
        	.cacert_len = sizeof(tuya_cacert_pem),
        	.device_id = deviceId,
        	.device_secret = deviceSecret,
        	.keepalive = 100,
        	.timeout_ms = 2000,
        	.on_connected = on_connected,
        	.on_disconnect = on_disconnect,
        	.on_messages = on_messages
    	});

    	ret = tuya_mqtt_connect(client);
    	if (ret != OPRT_OK) {
        	syslog(LOG_ERR, "ERROR: Failed to connect to Tuya service. Error code: %d", ret);
        	tuya_mqtt_disconnect(client);
        	closelog();
		ret = tuya_mqtt_deinit(client);
        	return ret;
    	}

    	while (g_signal_flag) {s
        	tuya_mqtt_loop(client);
    	}
    	
    	tuya_mqtt_disconnect(client);
	closelog ();
	ret = tuya_mqtt_deinit(client);

    	return ret;
}
