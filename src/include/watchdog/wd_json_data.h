/* -*-pgsql-c-*- */
/*
 *
 * $Header$
 *
 * pgpool: a language independent connection pool server for PostgreSQL
 * written by Tatsuo Ishii
 *
 * Copyright (c) 2003-2015	PgPool Global Development Group
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of the
 * author not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. The author makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 *
 */
#include "utils/json.h"
#include "pool_config.h"
#include "watchdog/watchdog.h"

#ifndef WD_JSON_DATA_H
#define WD_JSON_DATA_H

typedef struct WDNodeInfo
{
	int state;
	char nodeName[WD_MAX_HOST_NAMELEN];
	char hostName[WD_MAX_HOST_NAMELEN];		/* host name */
	char stateName[WD_MAX_HOST_NAMELEN];	/* watchdog state name */
	int wd_port;							/* watchdog port */
	int pgpool_port;						/* pgpool port */
	int wd_priority;						/* node priority */
	char delegate_ip[WD_MAX_HOST_NAMELEN];	/* delegate IP */
	int	id;
}WDNodeInfo;

extern WatchdogNode* get_watchdog_node_from_json(char* json_data, int data_len, char** authkey);
extern char* get_watchdog_node_info_json(WatchdogNode* wdNode, char* authkey);
extern POOL_CONFIG* get_pool_config_from_json(char* json_data, int data_len);
extern char* get_pool_config_json(void);
extern char* get_lifecheck_node_status_change_json(int nodeID, int nodeStatus, char* message, char* authKey);
extern bool parse_node_status_json(char* json_data, int data_len, int* nodeID, int* nodeStatus, char** message);

extern WDNodeInfo* get_WDNodeInfo_from_wd_node_json(json_value* source);

extern char* get_wd_node_function_json(char* func_name, int *node_id_set, int count, unsigned int sharedKey, char* authKey);
extern bool parse_wd_node_function_json(char* json_data, int data_len, char** func_name, int **node_id_set, int *count);
extern char* get_wd_simple_error_message_json(char* message);
#endif
