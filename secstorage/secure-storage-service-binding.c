/*
 * Copyright (C) 2016-2018 "IoT.bzh"
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <json-c/json.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

#include <db.h>

#define DB_FILE "secstorage.db"			  /* DB On-disk file name*/
#define DB_PASSWD_FILE "/tmp/test.passwd" /* DB PASSWD On-disk file name*/
static DB *dbp;							  /* DB structure handle */
static int global_storage;

#define DATA_SET(k, d, s)            \
	do                               \
	{                                \
		memset((k), 0, sizeof *(k)); \
		(k)->data = (void *)d;       \
		(k)->size = (uint32_t)s;     \
	} while (0)
#define DATA_PTR(k) ((void *)((k).data))
#define DATA_STR(k) ((char *)((k).data))
#define VALUE_MAX_LEN 4096

static int get_passwd(char *db_passwd)
{
	FILE *fp;

	fp = fopen(DB_PASSWD_FILE, "r");

	// test for files not existing.
	if (fp == NULL)
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to open DB file passwd: %s", DB_PASSWD_FILE);
		return -1;
	}

	AFB_API_NOTICE(afbBindingRoot, "DB DB_PASSWD_FILE: \"%s\"", DB_PASSWD_FILE);

	int res = fscanf(fp, "%s", db_passwd);

	if (res != 1)
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to get DB passwd.");
		return -1;
	}

	fclose(fp);

	return 0;
}

static int open_database(const char *path)
{
	char db_passwd[PATH_MAX];
	/* Initialize the structure. This
 	* database is not opened in an environment,
 	* so the environment pointer is NULL. */
	int res = db_create(&dbp, NULL, 0);

	if (res != 0)
	{
		AFB_API_ERROR(afbBindingRoot, "Failed database creation: %s.", db_strerror(res));
		return -1;
	}

	int res_pwd = get_passwd(db_passwd);
	if (res_pwd != 0)
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to get database password.");
		return -1;
	}

	AFB_API_NOTICE(afbBindingRoot, "DB passwd: \"%s\"", db_passwd); //RLM

	dbp->set_encrypt(dbp,			  /* DB structure pointer */
					 db_passwd,		  /*Passworld to  encryption/decryption of the database*/
					 DB_ENCRYPT_AES); /*Use the Rijndael/AES algorithm for encryption or decryption.*/

	res = dbp->open(dbp,	   /* DB structure pointer */
					NULL,	   /* Transaction pointer */
					path,	   /* On-disk file that holds the database. */
					NULL,	   /* Optional logical database name */
					DB_BTREE,  /* Database access method */
					DB_CREATE, /* Open flags */
					0600);	   /* File mode */

	if (res != 0)
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to open the '%s' database: %s.", path, db_strerror(res));
		dbp->close(dbp, 0);
		res = -1;
	}
	return res;
}

/**
 * @brief sync data base to physical storage
 */
static void secs_sync_database(void)
{
	/*Force sync each time*/
	dbp->sync(dbp, 0); /*Force Data Persistence*/
}

/**
 * @brief Get the database's path
 */
static int get_db_path(char *buffer, size_t size)
{
	char *afb_workdir;
	char *local_workdir;
	int res;

	afb_workdir = secure_getenv("AFB_WORKDIR");
	if (afb_workdir)
		res = snprintf(buffer, size, "%s/%s", afb_workdir, DB_FILE);
	else
	{
		AFB_API_WARNING(afbBindingRoot, "Failed to find AFB_WORKDIR");
		local_workdir = secure_getenv("PWD");
		if (local_workdir)
			res = snprintf(buffer, size, "%s/%s", local_workdir, DB_FILE);
		else
		{
			AFB_API_ERROR(afbBindingRoot, "Failed to find PWD");
			return -1;
		}
	}
	return res;
}

/**
 * Returns the database key for the 'req'
 */
static int get_key(afb_req_t req, DBT *key)
{
	char *appid;
	char *data;
	const char *req_key;

	size_t lreq_key;
	size_t lappid;
	size_t size;

	struct json_object *args;
	struct json_object *item;

	/* get the key */
	args = afb_req_json(req);
	if (!json_object_object_get_ex(args, "key", &item))
	{
		afb_req_reply(req, NULL, "no-key", NULL);
		return -1;
	}

	if (!item || !(req_key = json_object_get_string(item)) || !(lreq_key = strlen(req_key)))
	{
		afb_req_reply(req, NULL, "bad-key", NULL);
		return -1;
	}

	if (global_storage)
	{
		DATA_SET(key, req_key, lreq_key);
	}
	else
	{
		/* get the appid */
		appid = afb_req_get_application_id(req);
		if (!appid)
		{
			afb_req_reply(req, NULL, "bad-context", NULL);
			return -1;
		}

		/* make the db-key */
		lappid = strlen(appid);
		size = lappid + lreq_key + 2;
		data = realloc(appid, size);
		if (!data)
		{
			free(appid);
			afb_req_reply(req, NULL, "out-of-memory", NULL);
			return -1;
		}
		data[lappid] = '/';
		memcpy(&data[lappid + 1], req_key, lreq_key + 1);
		/* return the key */
		DATA_SET(key, data, size);
	}

	return 0;
}

/**
 * Returns the database value for the 'req'
 */
static int get_value(afb_req_t req, DBT *data)
{
	const char *value;

	struct json_object *args;
	struct json_object *item;

	/* get the value */
	args = afb_req_json(req);
	if (!json_object_object_get_ex(args, "value", &item))
	{
		afb_req_reply(req, NULL, "no-value", NULL);
		return -1;
	}
	value = json_object_get_string(item);
	if (!value)
	{
		afb_req_reply(req, NULL, "out-of-memory", NULL);
		return -1;
	}
	DATA_SET(data, value, strlen(value) + 1); /* includes the tailing null */

	return 0;
}

/**
 * @brief Initialize the binding.
 * @return Exit code, zero if success.
 */
static int init_secure_storage_binding(afb_api_t api)
{
	int res;
	char db_path[PATH_MAX];

	global_storage = 0;

	res = get_db_path(db_path, sizeof db_path);
	if (res < 0 || (int)res >= (int)(sizeof db_path))
	{
		AFB_API_ERROR(afbBindingRoot, "No database filename available");
		return -1;
	}

	AFB_API_INFO(afbBindingRoot, "open database: \"%s\"", db_path);
	return open_database(db_path);
}

/**
 * @brief API read
 */
static void secs_read(afb_req_t req)
{
	DBT key;
	DBT data;
	int ret;

	char value[VALUE_MAX_LEN];

	struct json_object *result;
	struct json_object *val;

	if (get_key(req, &key))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
		return;
	}

	AFB_API_DEBUG(afbBindingRoot, "read: key=%s", DATA_STR(key));

	memset(&data, 0, sizeof data);
	data.data = value;
	data.ulen = VALUE_MAX_LEN;
	data.flags = DB_DBT_USERMEM;

	ret = dbp->get(dbp, NULL, &key, &data, 0);
	if (ret == 0)
	{
		result = json_object_new_object();
		val = json_tokener_parse(DATA_STR(data));
		json_object_object_add(result, "value", val ? val : json_object_new_string(DATA_STR(data)));

		afb_req_reply_f(req, result, NULL, "db success: read %s=%s.", DATA_STR(key), DATA_STR(data));
	}
	else
	{
		afb_req_reply_f(req, NULL, "Failed to read datas.", "db fail: read %s - %s", DATA_STR(key), db_strerror(ret));
	}

	free(DATA_PTR(key));
}

/**
 * @brief API write
 */
static void secs_write(afb_req_t req)
{
	DBT key;
	DBT data;

	/* get the key */
	if (get_key(req, &key))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req key parameter");
		return;
	}

	/* get the value */
	if (get_value(req, &data))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req value parameter");
		return;
	}

	AFB_API_DEBUG(afbBindingRoot, "put: key=%s, value=%s", DATA_STR(key), DATA_STR(data));

	int res = dbp->put(dbp, NULL, &key, &data, DB_OVERWRITE_DUP);
	if (res == 0)
	{
		afb_req_reply(req, NULL, NULL, NULL);
	}
	else
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to insert %s with %s", DATA_STR(key), DATA_STR(data));
		afb_req_reply_f(req, NULL, "failed", "%s", db_strerror(res));
	}

	secs_sync_database();
}

/**
 * @brief API delete
 */
static void secs_delete(afb_req_t req)
{
	DBT key;

	if (get_key(req, &key))
		return;

	AFB_API_DEBUG(afbBindingRoot, "delete: key=%s", DATA_STR(key));

	int ret;

	ret = dbp->del(dbp, NULL, &key, 0);
	if (ret == 0)
	{
		afb_req_reply_f(req, NULL, NULL, NULL);
	}
	else
	{
		AFB_API_ERROR(afbBindingRoot, "can't delete key %s", DATA_STR(key));
		afb_req_reply_f(req, NULL, "failed", "%s", db_strerror(ret));
	}
}

static const struct afb_auth _afb_auths_v2_monitor[] = {
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:set"},
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:get"},
	{.type = afb_auth_Or, .first = &_afb_auths_v2_monitor[1], .next = &_afb_auths_v2_monitor[0]}};

static const afb_verb_t verbs[] = {
	{.verb = "read", .session = AFB_SESSION_NONE, .callback = secs_read, .auth = NULL},
	{.verb = "write", .session = AFB_SESSION_NONE, .callback = secs_write, .auth = NULL},
	{.verb = "delete", .session = AFB_SESSION_NONE, .callback = secs_delete, .auth = NULL},
	{NULL}};

const afb_binding_t afbBindingExport = {
	.api = "secstorage",
	.specification = NULL,
	.verbs = verbs,
	.preinit = NULL,
	.init = init_secure_storage_binding,
	.onevent = NULL,
	.userdata = NULL,
	.provide_class = NULL,
	.require_class = NULL,
	.require_api = NULL,
	.noconcurrency = 0};
