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
#include "arg_parser.h"

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
void on_connected(tuya_mqtt_context_t* context, void* user_data)
{
    	TY_LOGI("on connected");
    	syslog(LOG_INFO, "Connected to Tuya.");
}

void on_disconnect(tuya_mqtt_context_t* context, void* user_data)
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
            	
            	case THING_TYPE_PROPERTY_SET:
            		TY_LOGI("property set:%s", msg->data_string);

		case THING_TYPE_ACTION_EXECUTE:
			TY_LOGI("Action executed: %s", msg->data_string);
			cJSON *root = cJSON_Parse(msg->data_string);
			if (root == NULL) {
				syslog(LOG_ERR, "JSON parse was not successful");
			} else {
				syslog(LOG_INFO, "JSON parse was successful");
				transfer_data(context, msg, root);
			}
			break;	
		
        	default:
            		break;
    	}
    	printf("\r\n");
}

void transfer_data(tuya_mqtt_context_t* context, const tuyalink_message_t* msg, cJSON *root) {
	cJSON* input_params = cJSON_GetObjectItem(root, "inputParams");
	if (input_params != NULL) {
		cJSON *greeting_value = cJSON_GetObjectItem(input_params, "greeting_identifier");
		if (greeting_value != NULL) {
			char set_greeting[300];
			sprintf(set_greeting, "{\"greeting\":{\"value\":\"%s\",\"time\":1631708204231}}", greeting_value->valuestring);
			tuyalink_thing_property_report_with_ack(context, NULL, set_greeting);
		} else {
			syslog(LOG_ERR, "Failed to get greeting value from inputParams");
		}
		cJSON *test_value = cJSON_GetObjectItem(input_params, "test_bool_i");
		if (test_value != NULL) {
			bool test_bool = cJSON_IsTrue(test_value);
			if (test_bool) {
				tuyalink_thing_property_report_with_ack(context, NULL, "{\"daemon_test\":{\"value\":true,\"time\":1631708204231}}");
			} else {
				tuyalink_thing_property_report_with_ack(context, NULL, "{\"daemon_test\":{\"value\":false,\"time\":1631708204231}}");
			}
		} else {
			syslog(LOG_ERR, "Failed to get test bool value from inputParams");
		}
	} else {
		syslog(LOG_ERR, "Failed to get 'inputParams from JSON");
	}
}


int main(int argc, char** argv) {
	char productId[50];     
	char deviceId[50];      
	char deviceSecret[50]; 
	
	struct arguments arguments;
	tuya_mqtt_context_t client_instance;
	tuya_mqtt_context_t* client;
	int ret = OPRT_OK;
	
	setlogmask(LOG_UPTO (LOG_INFO));

	openlog("Daemon program to connect to Tuya", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	
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
        	goto cleanup;
    	}

    	while (g_signal_flag) {
        	tuya_mqtt_loop(client);
    	}
    	
    	cleanup:
    		tuya_mqtt_disconnect(client);
		closelog();
		ret = tuya_mqtt_deinit(client);

    	return ret;
}
