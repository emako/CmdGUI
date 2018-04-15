#ifndef IPC_DEFINES_H_INCLUDED
#define IPC_DEFINES_H_INCLUDED

// Watcher <-> Job server communication

static const char JOB_SERVER_NAME[] = "ws-server";
static const uint16_t JOB_SERVER_PORT = 3371;

// Client messages
static const char MSG_GET_JOBS_INFO[] = "GJI";
static const char MSG_GET_LOG[] = "GL";
static const char MSG_SUBSCRIBE[] = "SS";
static const char MSG_UNSUBSCRIBE[] = "USS";
static const char MSG_CLOSE_SERVER[] = "CS";
static const char MSG_GET_TRUSTED_CLIENTS[] = "GTC";
static const char MSG_SET_TRUSTED_CLIENTS[] = "STC";

static const char MSG_START_ALL_WAITING_JOBS[] = "SAWJ";
static const char MSG_PAUSE_ACTIVE_JOBS[] = "PACJ";
static const char MSG_RESUME_PAUSED_JOBS[] = "RPJ";
static const char MSG_ABORT_ACTIVE_JOBS[] = "AACJ";

// Server messages
static const char SMSG_LOG_MESSAGE[] = "LM";
static const char SMSG_JOBS_RUNNING[] = "JRUN";
static const char SMSG_JOBS_ABORTED[] = "JAED";
static const char SMSG_JOBS_PAUSED[] = "JPED";
static const char SMSG_JOBS_FAILED[] = "JFED";
static const char SMSG_JOBS_COMPLETED[] = "JCED";

static const char SMSG_CLOSING_SERVER[] = "SCS";
static const char SMSG_TRUSTED_CLIENTS_INFO[] = "TCI";

// Editor <-> Watcher communication

static const char JOB_SERVER_WATCHER_LOCAL_SERVER_NAME[] = "ws-client";

static const char WMSG_SHOW_WINDOW[] = "S";
static const char WMSG_CLI_ENCODE_JOB[] = "CEJ";

#endif // IPC_DEFINES_H_INCLUDED
