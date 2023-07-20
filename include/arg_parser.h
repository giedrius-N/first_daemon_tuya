#ifndef ARG_PARSER_H
#define ARG_PARSER_H

struct arguments {
    char prodId[50];
    char devId[50];
    char devSec[50];
    bool daemonize;
};

static struct argp_option options[] = {
    {"prodID", 'p', "STRING", 0, "DeviceID", 0},
    {"devId", 'd', "STRING", 0, "DeviceSecret", 0},
    {"devSec", 's', "STRING", 0, "ProductID", 0},
    {"daemon", 'D', 0, 0, "Daemonize the process", 0},
    {0}
};

static char args_doc[] = "DeviceID DeviceSecret ProductID";
static error_t parse_opt(int key, char *arg, struct argp_state *state);
static struct argp argp = {options, parse_opt, args_doc, NULL};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
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
                argp_usage(state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

#endif
