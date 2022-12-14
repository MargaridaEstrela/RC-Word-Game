#ifndef CONSTANTS_H
#define CONSTANTS_H

// SIZES
#define MAX_COMMAND_LINE 128
#define MAX_TCP_RESPONSE 45
#define MAX_TCP_READ 1024

// STATUS CODES
#define STATUS_OK 2
#define STATUS_WIN 3
#define STATUS_DUP 4
#define STATUS_NOK 5
#define STATUS_OVR 6
#define STATUS_INV 7
#define STATUS_ERR 8
#define STATUS_EMPTY 9
#define STATUS_ACT 10
#define STATUS_FIN 11

//TIMER
#define TIME_LIMIT 5

// ERROR STRINGS
#define ERR_UDP "UDP"
#define ERR_TCP "-1 TCP"
#define ERR_ERR "ERR"
#define ERR_LOST "LOST"

#endif // ONSTANTS_H
