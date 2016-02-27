/*
 * $Header$
 *
 * Handles watchdog connection, and protocol communication with pgpool-II
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
 */
#include <string.h>
#include <stdlib.h>

#include "utils/elog.h"
#include "utils/json_writer.h"
#include "utils/json.h"
#include "pool_config.h"
#include "watchdog/watchdog.h"
#include "watchdog/wd_json_data.h"
#include "watchdog/wd_ipc_defines.h"

POOL_CONFIG* get_pool_config_from_json(char* json_data, int data_len)
{
	int i;
	json_value *root = NULL;
	json_value *value = NULL;
	POOL_CONFIG *config = palloc0(sizeof(POOL_CONFIG));
	config->backend_desc = palloc0(sizeof(BackendDesc));

	root = json_parse(json_data,data_len);
	/* The root node must be object */
	if (root == NULL || root->type != json_object)
		goto ERROR_EXIT;

	if (json_get_int_value_for_key(root, "num_init_children", &config->num_init_children))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "listen_backlog_multiplier", &config->listen_backlog_multiplier))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "child_life_time", &config->child_life_time))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "connection_life_time", &config->connection_life_time))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "child_max_connections", &config->child_max_connections))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "client_idle_limit", &config->client_idle_limit))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "max_pool", &config->max_pool))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "replication_mode", &config->replication_mode))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "enable_pool_hba", &config->enable_pool_hba))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "load_balance_mode", &config->load_balance_mode))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "replication_stop_on_mismatch", &config->replication_stop_on_mismatch))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "failover_if_affected_tuples_mismatch", &config->failover_if_affected_tuples_mismatch))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "replicate_select", &config->replicate_select))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "master_slave_mode", &config->master_slave_mode))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "connection_cache", &config->connection_cache))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "health_check_timeout", &config->health_check_timeout))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "health_check_period", &config->health_check_period))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "health_check_max_retries", &config->health_check_max_retries))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "health_check_retry_delay", &config->health_check_retry_delay))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "fail_over_on_backend_error", &config->fail_over_on_backend_error))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "recovery_timeout", &config->recovery_timeout))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "search_primary_node_timeout", &config->search_primary_node_timeout))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "client_idle_limit_in_recovery", &config->client_idle_limit_in_recovery))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "insert_lock", &config->insert_lock))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "memory_cache_enabled", &config->memory_cache_enabled))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "use_watchdog", &config->use_watchdog))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "clear_memqcache_on_escalation", &config->clear_memqcache_on_escalation))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "wd_port", &config->wd_port))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "wd_priority", &config->wd_priority))
		goto ERROR_EXIT;

	config->master_slave_sub_mode = json_get_string_value_for_key(root, "master_slave_sub_mode");
	if (config->master_slave_sub_mode == NULL)
		goto ERROR_EXIT;
	config->master_slave_sub_mode = pstrdup(config->master_slave_sub_mode);


	/* backend_desc array */
	value = json_get_value_for_key(root,"backend_desc");
	if (value == NULL || value->type != json_array)
		goto ERROR_EXIT;

	config->backend_desc->num_backends = value->u.array.length;
	for (i = 0; i < config->backend_desc->num_backends; i++)
	{
		json_value *arr_value = value->u.array.values[i];
		char *ptr;
		if (json_get_int_value_for_key(arr_value, "backend_port", &config->backend_desc->backend_info[i].backend_port))
			goto ERROR_EXIT;
		ptr = json_get_string_value_for_key(arr_value, "backend_hostname");
		if (ptr == NULL)
			goto ERROR_EXIT;
		strncpy(config->backend_desc->backend_info[i].backend_hostname, ptr,sizeof(config->backend_desc->backend_info[i].backend_hostname) -1);
	}

	/* wd_remote_nodes array */
	value = json_get_value_for_key(root,"wd_remote_nodes");
	if (value == NULL || value->type != json_array)
		goto ERROR_EXIT;
	
	config->wd_remote_nodes.num_wd = value->u.array.length;
	for (i = 0; i < config->wd_remote_nodes.num_wd; i++)
	{
		json_value *arr_value = value->u.array.values[i];
		char *ptr;
		if (json_get_int_value_for_key(arr_value, "wd_port", &config->wd_remote_nodes.wd_remote_node_info[i].wd_port))
			goto ERROR_EXIT;
		if (json_get_int_value_for_key(arr_value, "pgpool_port", &config->wd_remote_nodes.wd_remote_node_info[i].pgpool_port))
			goto ERROR_EXIT;
		ptr = json_get_string_value_for_key(arr_value, "hostname");
		if (ptr == NULL)
			goto ERROR_EXIT;
		strncpy(config->wd_remote_nodes.wd_remote_node_info[i].hostname, ptr,sizeof(config->wd_remote_nodes.wd_remote_node_info[i].hostname) -1);
	}

	json_value_free(root);
	return config;

ERROR_EXIT:
	if (root)
		json_value_free(root);
	if (config->backend_desc)
		pfree(config->backend_desc);
	if (config->master_slave_sub_mode)
		pfree(config->master_slave_sub_mode);
	pfree(config);
	return NULL;
}

char* get_pool_config_json(void)
{
	int i;
	char* json_str;

	JsonNode* jNode = jw_create_with_object(true);

	jw_put_int(jNode, "num_init_children", pool_config->num_init_children);
	jw_put_int(jNode, "listen_backlog_multiplier", pool_config->listen_backlog_multiplier);
	jw_put_int(jNode, "child_life_time", pool_config->child_life_time);
	jw_put_int(jNode, "connection_life_time", pool_config->connection_life_time);
	jw_put_int(jNode, "child_max_connections", pool_config->child_max_connections);
	jw_put_int(jNode, "client_idle_limit", pool_config->client_idle_limit);
	jw_put_int(jNode, "max_pool", pool_config->max_pool);
	jw_put_int(jNode, "replication_mode", pool_config->replication_mode);
	jw_put_int(jNode, "enable_pool_hba", pool_config->enable_pool_hba);
	jw_put_int(jNode, "load_balance_mode", pool_config->load_balance_mode);
	jw_put_int(jNode, "replication_stop_on_mismatch", pool_config->replication_stop_on_mismatch);
	jw_put_int(jNode, "failover_if_affected_tuples_mismatch", pool_config->failover_if_affected_tuples_mismatch);
	jw_put_int(jNode, "replicate_select", pool_config->replicate_select);
	jw_put_int(jNode, "master_slave_mode", pool_config->master_slave_mode);
	jw_put_int(jNode, "connection_cache", pool_config->connection_cache);
	jw_put_int(jNode, "health_check_timeout", pool_config->health_check_timeout);
	jw_put_int(jNode, "health_check_period", pool_config->health_check_period);
	jw_put_int(jNode, "health_check_max_retries", pool_config->health_check_max_retries);
	jw_put_int(jNode, "health_check_retry_delay", pool_config->health_check_retry_delay);
	jw_put_int(jNode, "fail_over_on_backend_error", pool_config->fail_over_on_backend_error);
	jw_put_int(jNode, "recovery_timeout", pool_config->recovery_timeout);
	jw_put_int(jNode, "search_primary_node_timeout", pool_config->search_primary_node_timeout);
	jw_put_int(jNode, "client_idle_limit_in_recovery", pool_config->client_idle_limit_in_recovery);
	jw_put_int(jNode, "insert_lock", pool_config->insert_lock);
	jw_put_int(jNode, "memory_cache_enabled", pool_config->memory_cache_enabled);
	jw_put_int(jNode, "use_watchdog", pool_config->use_watchdog);
	jw_put_int(jNode, "clear_memqcache_on_escalation", pool_config->clear_memqcache_on_escalation);
	jw_put_int(jNode, "wd_port", pool_config->wd_port);
	jw_put_int(jNode, "wd_priority", pool_config->wd_priority);
	
	jw_put_string(jNode, "master_slave_sub_mode", pool_config->master_slave_sub_mode);

	/* Array of backends */
	jw_start_array(jNode, "backend_desc");
	for (i=0; i < pool_config->backend_desc->num_backends; i++)
	{
		jw_start_object(jNode, "BackendInfo");
		jw_put_string(jNode, "backend_hostname",pool_config->backend_desc->backend_info[i].backend_hostname);
		jw_put_int(jNode, "backend_port", pool_config->backend_desc->backend_info[i].backend_port);
		jw_end_element(jNode);
	}
	jw_end_element(jNode); /* backend_desc array End */

	/* Array of wd_remote_nodes */
	jw_start_array(jNode, "wd_remote_nodes");
	for (i=0; i < pool_config->wd_remote_nodes.num_wd; i++)
	{
		jw_start_object(jNode, "WdRemoteNodesConfig");
		jw_put_string(jNode, "hostname",pool_config->wd_remote_nodes.wd_remote_node_info[i].hostname);
		jw_put_int(jNode, "wd_port", pool_config->wd_remote_nodes.wd_remote_node_info[i].wd_port);
		jw_put_int(jNode, "pgpool_port", pool_config->wd_remote_nodes.wd_remote_node_info[i].pgpool_port);
		jw_end_element(jNode);
	}
	jw_end_element(jNode); /* wd_remote_nodes array End */

	jw_finish_document(jNode);
	json_str = pstrdup(jw_get_json_string(jNode));
	jw_destroy(jNode);
	return json_str;
}


char* get_watchdog_node_info_json(WatchdogNode* wdNode, char* authkey)
{
	char* json_str;
	JsonNode* jNode = jw_create_with_object(true);

	jw_put_long(jNode, "StartupTimeSecs", wdNode->startup_time.tv_sec);
	jw_put_int(jNode, "State", wdNode->state);
	jw_put_int(jNode, "WdPort", wdNode->wd_port);
	jw_put_int(jNode, "PgpoolPort", wdNode->pgpool_port);
	jw_put_int(jNode, "WdPriority", wdNode->wd_priority);

	jw_put_string(jNode, "NodeName",wdNode->nodeName);
	jw_put_string(jNode, "HostName",wdNode->hostname);
	jw_put_string(jNode, "VIP",wdNode->delegate_ip);

	if(authkey)
		jw_put_string(jNode, "authkey",authkey);

	jw_finish_document(jNode);
	json_str = pstrdup(jw_get_json_string(jNode));
	jw_destroy(jNode);
	return json_str;

}

WatchdogNode* get_watchdog_node_from_json(char* json_data, int data_len, char** authkey)
{
	json_value *root = NULL;
	char* ptr;
	WatchdogNode* wdNode = palloc0(sizeof(WatchdogNode));

	root = json_parse(json_data,data_len);
	/* The root node must be object */
	if (root == NULL || root->type != json_object)
		goto ERROR_EXIT;

	if (json_get_long_value_for_key(root, "StartupTimeSecs", &wdNode->startup_time.tv_sec))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "State", (int*)&wdNode->state))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "WdPort", &wdNode->wd_port))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "PgpoolPort", &wdNode->pgpool_port))
		goto ERROR_EXIT;
	if (json_get_int_value_for_key(root, "WdPriority", &wdNode->wd_priority))
		goto ERROR_EXIT;


	ptr = json_get_string_value_for_key(root, "NodeName");
	if (ptr == NULL)
		goto ERROR_EXIT;
	strncpy(wdNode->nodeName, ptr, sizeof(wdNode->nodeName) -1);

	ptr = json_get_string_value_for_key(root, "HostName");
	if (ptr == NULL)
		goto ERROR_EXIT;
	strncpy(wdNode->hostname, ptr, sizeof(wdNode->hostname) -1);

	ptr = json_get_string_value_for_key(root, "VIP");
	if (ptr == NULL)
		goto ERROR_EXIT;
	strncpy(wdNode->delegate_ip, ptr, sizeof(wdNode->delegate_ip) -1);

	if (authkey)
	{
		ptr = json_get_string_value_for_key(root, "authkey");
		if (ptr != NULL)
			*authkey = pstrdup(ptr);
		else
			*authkey = NULL;
	}
	return wdNode;

	ERROR_EXIT:
	if (root)
		json_value_free(root);
	pfree(wdNode);
	return NULL;
}


char* get_lifecheck_node_status_change_json(int nodeID, int nodeStatus, char* message, char* authKey)
{
	char* json_str;
	JsonNode* jNode = jw_create_with_object(true);

	if (authKey != NULL && strlen(authKey) > 0)
		jw_put_string(jNode, WD_IPC_AUTH_KEY, authKey); /*  put the auth key*/

	/* add the node ID */
	jw_put_int(jNode, "NodeID", nodeID);
	/* add the node status */
	jw_put_int(jNode, "NodeStatus",nodeStatus);
	/* add the node message if any */
	if (message)
		jw_put_string(jNode, "Message", message);

	jw_finish_document(jNode);
	json_str = pstrdup(jw_get_json_string(jNode));
	jw_destroy(jNode);
	return json_str;
}

bool parse_node_status_json(char* json_data, int data_len, int* nodeID, int* nodeStatus, char** message)
{
	json_value* root;
	char* ptr = NULL;
	root = json_parse(json_data,data_len);

	/* The root node must be object */
	if (root == NULL || root->type != json_object)
		goto ERROR_EXIT;

	if (json_get_int_value_for_key(root, "NodeID", nodeID))
		goto ERROR_EXIT;

	if (json_get_int_value_for_key(root, "NodeStatus", nodeStatus))
		goto ERROR_EXIT;

	ptr = json_get_string_value_for_key(root, "Message");
	if (ptr != NULL)
		*message = pstrdup(ptr);

	json_value_free(root);
	return true;

ERROR_EXIT:
	if (root)
		json_value_free(root);
	return false;
}

WDNodeInfo* get_WDNodeInfo_from_wd_node_json(json_value* source)
{
	char* ptr;
	WDNodeInfo* wdNodeInfo = palloc0(sizeof(WDNodeInfo));
	if (source->type != json_object)
		ereport(ERROR,
			(errmsg("invalid json data"),
				 errdetail("node is not of object type")));
	
	if (json_get_int_value_for_key(source, "ID", &wdNodeInfo->id))
	{
		ereport(ERROR,
			(errmsg("invalid json data"),
				 errdetail("unable to find Watchdog Node ID")));
	}
	
	ptr = json_get_string_value_for_key(source, "NodeName");
	if (ptr == NULL)
	{
		ereport(ERROR,
			(errmsg("invalid json data"),
				 errdetail("unable to find Watchdog Node Name")));
	}
	strncpy(wdNodeInfo->nodeName, ptr, sizeof(wdNodeInfo->nodeName) -1);
	
	ptr = json_get_string_value_for_key(source, "HostName");
	if (ptr == NULL)
	{
		ereport(ERROR,
			(errmsg("invalid json data"),
				 errdetail("unable to find Watchdog Host Name")));
	}
	strncpy(wdNodeInfo->hostName, ptr, sizeof(wdNodeInfo->hostName) -1);
	
	ptr = json_get_string_value_for_key(source, "DelegateIP");
	if (ptr == NULL)
	{
		ereport(ERROR,
			(errmsg("invalid json data"),
				 errdetail("unable to find Watchdog delegate IP")));
	}
	strncpy(wdNodeInfo->delegate_ip, ptr, sizeof(wdNodeInfo->delegate_ip) -1);
	
	if (json_get_int_value_for_key(source, "WdPort", &wdNodeInfo->wd_port))
	{
		ereport(ERROR,
				(errmsg("invalid json data"),
				 errdetail("unable to find WdPort")));
	}
	
	if (json_get_int_value_for_key(source, "PgpoolPort", &wdNodeInfo->pgpool_port))
	{
		ereport(ERROR,
				(errmsg("invalid json data"),
				 errdetail("unable to find PgpoolPort")));
	}

	if (json_get_int_value_for_key(source, "State", &wdNodeInfo->state))
	{
		ereport(ERROR,
				(errmsg("invalid json data"),
				 errdetail("unable to find state")));
	}

	ptr = json_get_string_value_for_key(source, "StateName");
	if (ptr == NULL)
	{
		ereport(ERROR,
			(errmsg("invalid json data"),
				 errdetail("unable to find Watchdog State Name")));
	}
	strncpy(wdNodeInfo->stateName, ptr, sizeof(wdNodeInfo->stateName) -1);

	if (json_get_int_value_for_key(source, "Priority", &wdNodeInfo->wd_priority))
	{
		ereport(ERROR,
				(errmsg("invalid json data"),
				 errdetail("unable to find state")));
	}
	
	return wdNodeInfo;
	
}

char* get_wd_node_function_json(char* func_name, int *node_id_set, int count, unsigned int sharedKey, char* authKey)
{
	char* json_str;
	int  i;
	JsonNode* jNode = jw_create_with_object(true);

	jw_put_int(jNode, WD_IPC_SHARED_KEY, sharedKey); /* put the shared key*/

	if (authKey != NULL && strlen(authKey) > 0)
		jw_put_string(jNode, WD_IPC_AUTH_KEY, authKey); /*  put the auth key*/

	jw_put_string(jNode, "Function", func_name);
	jw_put_int(jNode, "NodeCount", count);
	if (count > 0)
	{
		jw_start_array(jNode, "NodeIdList");
		for (i=0; i < count; i++) {
			jw_put_int_value(jNode, node_id_set[i]);
		}
		jw_end_element(jNode);
	}
	jw_finish_document(jNode);
	json_str = pstrdup(jw_get_json_string(jNode));
	jw_destroy(jNode);
	return json_str;
}

bool parse_wd_node_function_json(char* json_data, int data_len, char** func_name, int **node_id_set, int *count)
{
	json_value *root, *value;
	char* ptr;
	int node_count = 0;
	int i;

	*node_id_set = NULL;
	*func_name = NULL;
	*count = 0;

	root = json_parse(json_data,data_len);

	/* The root node must be object */
	if (root == NULL || root->type != json_object)
	{
		json_value_free(root);
		ereport(LOG,
			(errmsg("watchdog is unable to parse node function json"),
				 errdetail("invalid json data \"%s\"",json_data)));
		return false;
	}
	ptr = json_get_string_value_for_key(root, "Function");
	if (ptr == NULL)
	{
		json_value_free(root);
		ereport(LOG,
			(errmsg("watchdog is unable to parse node function json"),
				 errdetail("function node not found in json data \"%s\"",json_data)));
		return false;
	}
	*func_name = pstrdup(ptr);
	/* If it is a node function ?*/
	if (json_get_int_value_for_key(root, "NodeCount", &node_count))
	{
		/*node count not found, But we don't care much about this*/
		json_value_free(root);
		return true;
	}
	if (node_count <= 0)
	{
		json_value_free(root);
		return true;
	}
	*count = node_count;

	value = json_get_value_for_key(root,"NodeIdList");
	if (value == NULL)
	{
		json_value_free(root);
		ereport(LOG,
			(errmsg("invalid json data"),
				 errdetail("unable to find NodeIdList node from data")));
		return false;
	}
	if (value->type != json_array)
	{
		json_value_free(root);
		ereport(WARNING,
				(errmsg("invalid json data"),
				 errdetail("NodeIdList node does not contains Array")));
		return false;
	}
	if (node_count != value->u.array.length)
	{
		json_value_free(root);
		ereport(WARNING,
				(errmsg("invalid json data"),
				 errdetail("NodeIdList array contains %d nodes while expecting %d",value->u.array.length, node_count)));
		return false;
	}

	*node_id_set = palloc(sizeof(int) * node_count);
	for (i = 0; i < node_count; i++)
	{
		*node_id_set[i] = value->u.array.values[i]->u.integer;
	}
	json_value_free(root);
	return true;
}

char* get_wd_simple_error_message_json(char* message)
{
	char* json_str;
	JsonNode* jNode = jw_create_with_object(true);

	jw_put_string(jNode, "ERROR", message);
	jw_finish_document(jNode);
	json_str = pstrdup(jw_get_json_string(jNode));
	jw_destroy(jNode);
	return json_str;
}

