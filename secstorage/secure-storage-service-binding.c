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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

#include <libdb4/db.h>

//TODO use SECSTOREADMIN to activate Admin support
#ifndef ALLOW_SECS_ADMIN
#define ALLOW_SECS_ADMIN 1
#endif

#ifndef ALLOW_SECS_GLOBAL
#define ALLOW_SECS_GLOBAL 1
#endif

#define DB_FILE "secstorage.db"			  /* DB On-disk file name*/
#define DB_PASSWD_FILE "/tmp/test.passwd" /* DB PASSWD On-disk file name*/
static DB *dbp;
/* DB structure handle */

#define DATA_PTR(k) ((void *)((k).data))
#define DATA_STR(k) ((char *)((k).data))

#define VALUE_MAX_LEN 8192
#define KEY_MAX_LEN 255
#define DB_MAX_SIZE 16777216

#define DATA_SET(k, d, s)            \
	do                               \
	{                                \
		memset((k), 0, sizeof *(k)); \
		(k)->data = (void *)d;       \
		(k)->size = (uint32_t)s;     \
	} while (0)

#ifdef ALLOW_SECS_ADMIN
static DBC *cursor;
static char cursor_key_pass[KEY_MAX_LEN] = "";
#endif

#ifdef ALLOW_SECS_GLOBAL
#define GLOBAL_PATH "/global"
#endif

/**
* 
*/
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

	int res = fscanf(fp, "%s", db_passwd);

	if (res != 1)
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to get DB passwd.");
		return -1;
	}

	fclose(fp);

	return 0;
}

/**
* Open the secure store data base
*/
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
	/*
	Get the password to encrypte the database.
	*/
	int res_pwd = get_passwd(db_passwd);
	if (res_pwd != 0)
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to get database password.");
		return -1;
	}
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
 * Sync data base to physical storage
 */
static void secs_sync_database(void)
{
	/*Force sync each time*/
	dbp->sync(dbp, 0); /*Force Data Persistence*/
}

/**
 * Get the database's path
 */
static int get_db_path(char *buffer, size_t size)
{
	char *afb_workdir;
	char *local_workdir;
	int res;

	afb_workdir = secure_getenv("AFB_WORKDIR");
	if (afb_workdir)
	{
		res = snprintf(buffer, size, "%s/%s", afb_workdir, DB_FILE);
	}
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
static int get_rawkey(afb_req_t req, const char **req_key)
{
	size_t lreq_key;

	struct json_object *args;
	struct json_object *item;
	/* get the key value of the req*/
	args = afb_req_json(req);
	if (!json_object_object_get_ex(args, "key", &item))
	{
		afb_req_reply(req, NULL, "no-key", NULL);
		return -1;
	}
	/* Convert the req key value to string*/
	if (!item || !((*req_key) = json_object_get_string(item)) || !(lreq_key = strlen(*req_key)))
	{
		afb_req_reply(req, NULL, "bad-key", NULL);
		return -1;
	}
	return 0;
}

/**
 * Returns the database key for the 'req'
 */
static int get_path(afb_req_t req, char *path)
{
	size_t lreq_key;
	const char *req_key;
	struct json_object *args;
	struct json_object *item;
	/* get the key of the req*/
	args = afb_req_json(req);
	if (!json_object_object_get_ex(args, "path", &item))
	{
		afb_req_reply(req, NULL, "no-path", NULL);
		return -1;
	}
	/* Convert the req path value to string*/
	if (!item || !(req_key = json_object_get_string(item)) || !(lreq_key = strlen(req_key)))
	{
		afb_req_reply(req, NULL, "bad-path", NULL);
		return -1;
	}
	strcpy(path, req_key);
	return 0;
}

#ifdef ALLOW_SECS_ADMIN
/**
* check_path_end:
*	0: The path must end with a "/"
*	1: The path must not end with a "/"
*	2: Do not Check the end of the path.
*/
static int get_admin_key(afb_req_t req, char *data, int check_path_end)
{
	const char *req_key = NULL;
	if (get_rawkey(req, &req_key))
	{
		return -1;
	}

	if (req_key[0] != '/')
	{
		AFB_API_ERROR(afbBindingRoot, "Admin path requested must start with \"%s\"", "/");
		afb_req_reply(req, NULL, "Admin path must be absolute", NULL);
		return -1;
	}

	if ((check_path_end != 2) && !check_path_end ^ (req_key[strlen(req_key) - 1] == '/'))
	{
		if (check_path_end)
		{ //The key name can contain path separators, '/', to
			//* help organize an app's data.  Key names cannot end with a trailing separator.
			AFB_API_ERROR(afbBindingRoot, "Forbiden use of char \"%s\" at the end of key request", "/");
			afb_req_reply(req, NULL, "Forbiden char at the end of key request", NULL);
			return -1;
		}
		else
		{
			AFB_API_ERROR(afbBindingRoot, "Path requested must end with \"%s\"", "/");
			afb_req_reply(req, NULL, "Forbiden Path format", NULL);
			return -1;
		}
	}
	strcpy(data, req_key);
	return 0;
}
#endif

/**
 * Returns the database key for the 'req'
 */
static int get_key(afb_req_t req, char *data, int global)
{
	char *appid;
	const char *req_key;

	if (get_rawkey(req, &req_key))
	{
		return -1;
	}

	//The key name can contain path separators, '/', to
	//* help organize an app's data.  Key names cannot contain a trailing separator.
	if (req_key[strlen(req_key) - 1] == '/')
	{
		AFB_API_ERROR(afbBindingRoot, "Forbiden use of char \"%s\" at the end of key request", "/");
		afb_req_reply(req, NULL, "Forbiden char at the end of key request", NULL);
		return -1;
	}
	AFB_API_NOTICE(afbBindingRoot, "RLM data 1 to \"%s\".", data);
	if (global)
	{
		appid = GLOBAL_PATH;
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
		if (!strcat(data, "/"))
		{
			afb_req_reply(req, NULL, "first char of the key generation failed", NULL);
			return -1;
		}
	}
	if (!strcat(data, appid))
	{
		afb_req_reply(req, NULL, "key generation from application id failed", NULL);
		return -1;
	}
	if (req_key[0] != '/' && !strcat(data, "/"))
	{
		afb_req_reply(req, NULL, "key generation failed", NULL);
		return -1;
	}
	if (!strcat(data, req_key))
	{
		afb_req_reply(req, NULL, "key generation from json key failed", NULL);
		return -1;
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
 * API read
 */
static void p_secs_raw_read(afb_req_t req, DBT *key)
{
	DBT data;
	memset(&data, 0, sizeof(DBT));
	int res;

	unsigned char value[VALUE_MAX_LEN] = "";

	struct json_object *result;
	struct json_object *val;

	AFB_API_DEBUG(afbBindingRoot, "read: key=%s", DATA_STR(*key));

	data.data = value;
	data.ulen = VALUE_MAX_LEN;
	data.flags = DB_DBT_USERMEM;

	res = dbp->get(dbp, NULL, key, &data, 0);
	if (res == 0)
	{
		result = json_object_new_object();
		val = json_tokener_parse(DATA_STR(data));
		json_object_object_add(result, "value", val ? val : json_object_new_string(DATA_STR(data)));

		afb_req_reply_f(req, result, NULL, "db success: read %s=%s.", DATA_STR(*key), DATA_STR(data));
	}
	else
	{
		afb_req_reply_f(req, NULL, "Failed to read datas.", "db fail: read key %s - %s\n", DATA_STR(*key), db_strerror(res));
	}
}

/**
 * API write
 */
static void p_secs_raw_write(afb_req_t req, DBT *key, DBT *data)
{
	AFB_API_DEBUG(afbBindingRoot, "put: key=\"%s\", value=\"%s\"", DATA_STR(*key), DATA_STR(*data));

	int res = dbp->put(dbp, NULL, key, data, DB_OVERWRITE_DUP);

	if (res == 0)
	{
		afb_req_reply(req, NULL, NULL, NULL);
	}
	else
	{
		AFB_API_ERROR(afbBindingRoot, "Failed to insert %s with %s", DATA_STR(*key), DATA_STR(*data));
		afb_req_reply_f(req, NULL, "failed", "%s", db_strerror(res));
	}
	secs_sync_database();
	return;
}

/**
 * API delete
 */
static void p_secs_raw_delete(afb_req_t req, DBT *key)
{

	AFB_API_DEBUG(afbBindingRoot, "delete: key=%s", DATA_STR(*key));

	int ret;
	ret = dbp->del(dbp, NULL, key, 0);
	if (ret == 0)
	{
		afb_req_reply_f(req, NULL, NULL, NULL);
	}
	else
	{
		AFB_API_ERROR(afbBindingRoot, "can't delete key %s", DATA_STR(*key));
		afb_req_reply_f(req, NULL, "failed", "%s", db_strerror(ret));
	}

	secs_sync_database();
}

/**
 * API read
 */
static void p_secs_read(afb_req_t req, int global)
{
	DBT key;
	char kdata[KEY_MAX_LEN] = "";

	if (get_key(req, kdata, global))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
	}
	DATA_SET(&key, kdata, strlen(kdata) + 1);
	p_secs_raw_read(req, &key);
}

/**
 * API write
 */
static void p_secs_write(afb_req_t req, int global)
{
	DBT key;
	DBT value;
	memset(&value, 0, sizeof(DBT));
	char kdata[KEY_MAX_LEN] = "";
	/* get the key */
	if (get_key(req, kdata, global))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req key parameter");
		return;
	}

	/* get the value */
	if (get_value(req, &value))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req value parameter");
		return;
	}
	DATA_SET(&key, kdata, strlen(kdata) + 1);
	p_secs_raw_write(req, &key, &value);
}

/**
 * API delete
 */
static void p_secs_delete(afb_req_t req, int global)
{
	DBT key;
	char kdata[KEY_MAX_LEN] = "";

	if (get_key(req, kdata, global))
	{
		return;
	}
	DATA_SET(&key, kdata, strlen(kdata) + 1);
	p_secs_raw_delete(req, &key);
}

//--------------------------------------------------------------------------------------------------

/**
 * Writes an item to secure storage. If the item already exists, it'll be overwritten with
 * the new value. If the item doesn't already exist, it'll be created.
 * If the item name is not valid or the buffer is NULL, this function will kill the calling client.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there isn't enough memory to store the item.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secs_Write(afb_req_t req)
{
	p_secs_write(req, 0);
}

/**
 * Reads an item from secure storage.
 * If the item name is not valid or the buffer is NULL, this function will kill the calling client.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entire item. No data will be written to
 *                  the buffer in this case.
 *      LE_NOT_FOUND if the item doesn't exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secs_Read(afb_req_t req)
{
	p_secs_read(req, 0);
}

/**
 * Deletes an item from secure storage.
 * If the item name is not valid, this function will kill the calling client.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the item doesn't exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secs_Delete(afb_req_t req)
{
	p_secs_delete(req, 0);
}
//--------------------------------------------------------------------------------------------------

#ifdef ALLOW_SECS_GLOBAL
//--------------------------------------------------------------------------------------------------

/**
 * Writes an item to secure storage. If the item already exists, it'll be overwritten with
 * the new value. If the item doesn't already exist, it'll be created.
 * If the item name is not valid or the buffer is NULL, this function will kill the calling client.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there isn't enough memory to store the item.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secs_Write_global(afb_req_t req)
{
	p_secs_write(req, 1);
}

/**
 * Reads an item from secure storage.
 * If the item name is not valid or the buffer is NULL, this function will kill the calling client.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entire item. No data will be written to
 *                  the buffer in this case.
 *      LE_NOT_FOUND if the item doesn't exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secs_Read_global(afb_req_t req)
{
	p_secs_read(req, 1);
}

/**
 * Deletes an item from secure storage.
 * If the item name is not valid, this function will kill the calling client.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the item doesn't exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secs_Delete_global(afb_req_t req)
{
	p_secs_delete(req, 1);
}
//--------------------------------------------------------------------------------------------------
#endif

long int getdbsize()
{
	char db_path[PATH_MAX] = "";

	int res = get_db_path(db_path, sizeof db_path);
	if (res < 0 || (int)res >= (int)(sizeof db_path))
	{
		return -1;
	}
	FILE *fp;
	secs_sync_database();
	fp = fopen(db_path, "r");

	fseek(fp, 0L, SEEK_END);

	return ftell(fp);
}

/*

@return
* 0 if first_key != second_second
* 1 if second_second start with first_key
* 2 if first_key = second_second
*/
int compare_key_path(char *first_key, char *second_second)
{
	while (*first_key == *second_second)
	{
		if (*first_key == '\0' || *second_second == '\0')
			break;

		first_key++;
		second_second++;
	}

	if (*first_key == '\0')
	{
		if (*second_second == '\0')
		{
			return 2;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

//--------------------------------------------------------------------------------------------------
#ifdef ALLOW_SECS_ADMIN
static int copy_db_file(const char *from, const char *to)
{
	AFB_API_NOTICE(afbBindingRoot, "copy %s to %s.", to, from);
	int fd_to, fd_from;
	char buf[4096];
	ssize_t nread;
	int saved_errno;

	fd_from = open(from, O_RDONLY);
	if (fd_from < 0)
	{
		AFB_API_NOTICE(afbBindingRoot, "Open %s failed", from);
		return -1;
	}

	fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd_to < 0)
	{
		goto out_error;
	}

	AFB_API_NOTICE(afbBindingRoot, "Start copy");
	while (nread = read(fd_from, buf, sizeof buf), nread > 0)
	{
		char *out_ptr = buf;
		ssize_t nwritten;
		do
		{
			nwritten = write(fd_to, out_ptr, nread);

			if (nwritten >= 0)
			{
				nread -= nwritten;
				out_ptr += nwritten;
			}
			else if (errno != EINTR)
			{
				goto out_error;
			}
		} while (nread > 0);
	}

	AFB_API_NOTICE(afbBindingRoot, "End copy");
	if (nread == 0)
	{
		if (close(fd_to) < 0)
		{
			fd_to = -1;
			goto out_error;
		}
		close(fd_from);

		/* Success! */
		return 0;
	}

out_error:
	AFB_API_NOTICE(afbBindingRoot, "out_error");
	saved_errno = errno;

	close(fd_from);
	if (fd_to >= 0)
		close(fd_to);

	errno = saved_errno;
	return -1;
}

/**
 * Create an iterator for listing entries in secure storage under the specified path.
 *
 * @return
 *      An iterator reference if successful.
 *      NULL if the secure storage is currently unavailable.
 */
static void secStoreAdmin_CreateIter(afb_req_t req)
{
	/*If cursor is set to a old value close it*/
	if (cursor)
	{
		cursor->close(cursor);
	}
	/* cursor_key_pass must be set at least at "/" */
	if (get_admin_key(req, cursor_key_pass, 0))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
		return;
	}

	if (dbp->cursor(dbp, NULL, &cursor, 0))
	{
		AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_createiter:Failed to init cursor");
		return;
	}
	struct json_object *result = json_object_new_object();
	json_object_object_add(result, "iterator", json_object_new_int64(1));

	afb_req_reply(req, result, NULL, NULL);
}

/**
 * Deletes an iterator.
 */
static void secStoreAdmin_DeleteIter(afb_req_t req)
{
	if (cursor->del(cursor, 0))
	{
		AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_deleteIter:Failed to delete cursor");
		return;
	}
	else
	{
		afb_req_reply(req, NULL, NULL, NULL);
	}
}

/**
 * Go to the next entry in the iterator.  This should be called at least once before accessing the
 * entry.  After the first time this function is called successfully on an iterator the first entry
 * will be available.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no more entries available.
 */
static void secStoreAdmin_Next(afb_req_t req)
{
	DBT ckey;
	DBT cvalue;
	memset(&ckey, 0, sizeof(DBT));
	memset(&cvalue, 0, sizeof(DBT));

	while (!cursor->get(cursor, &ckey, &cvalue, DB_NEXT))
	{
		if (compare_key_path(cursor_key_pass, DATA_STR(ckey)) > 0)
		{
			break;
		}
	}
	afb_req_reply(req, NULL, NULL, NULL);
}

/**
 * Get the current entry's name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entry name.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 */
static void secStoreAdmin_GetEntry(afb_req_t req)
{
	DBT ckey;
	DBT cvalue;
	int res;
	struct json_object *result;
	struct json_object *val;

	memset(&ckey, 0, sizeof(DBT));
	memset(&cvalue, 0, sizeof(DBT));
	res = cursor->get(cursor, &ckey, &cvalue, DB_CURRENT);

	if (res == 0)
	{
		result = json_object_new_object();
		val = json_tokener_parse(DATA_STR(cvalue));
		json_object_object_add(result, "value", val ? val : json_object_new_string(DATA_STR(ckey)));

		afb_req_reply(req, result, NULL, NULL);
	}
	else
	{
		afb_req_reply_f(req, NULL, "Failed to read datas.", "db fail: read current cursor value - %s\n", db_strerror(res));
	}
}

/**
 * Writes a buffer of data into the specified path in secure storage.  If the item already exists,
 * it'll be overwritten with the new value. If the item doesn't already exist, it'll be created.
 *
 * @note
 *      The specified path must be an absolute path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NO_MEMORY if there isn't enough memory to store the item.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secStoreAdmin_Write(afb_req_t req)
{
	DBT key;
	DBT value;
	char kdata[KEY_MAX_LEN] = "";

	memset(&value, 0, sizeof(DBT));

	/* get the key */
	if (get_admin_key(req, kdata, 1))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req key parameter");
		return;
	}
	/* get the value */
	if (get_value(req, &value))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req value parameter");
		return;
	}
	DATA_SET(&key, kdata, strlen(kdata) + 1);
	p_secs_raw_write(req, &key, &value);
}

/**
 * Reads an item from secure storage.
 *
 * @note
 *      The specified path must be an absolute path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entire item. No data will be written to
 *                  the buffer in this case.
 *      LE_NOT_FOUND if the item doesn't exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secStoreAdmin_Read(afb_req_t req)
{
	DBT key;
	char kdata[KEY_MAX_LEN] = "";

	memset(&key, 0, sizeof(DBT));
	memset(&key, 0, sizeof(DBT));
	if (get_admin_key(req, kdata, 1))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
		return;
	}
	DATA_SET(&key, kdata, strlen(kdata) + 1);
	p_secs_raw_read(req, &key);
	return;
}

/**
 * Copy the meta file to the specified path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the meta file does not exist.
 *      LE_UNAVAILABLE if the sfs is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secStoreAdmin_CopyMetaTo(afb_req_t req)
{
	secs_sync_database();
	char db_path[PATH_MAX] = "";
	char db_path_dest[KEY_MAX_LEN] = "";

	if (get_path(req, db_path_dest))
	{
		return;
	}
	int res = get_db_path(db_path, sizeof db_path);
	if (res < 0 || (int)res >= (int)(sizeof db_path))
	{
		return;
	};

	copy_db_file(db_path, db_path_dest);
	afb_req_reply(req, NULL, NULL, NULL);
}

/**
 * Recursively deletes all items under the specified path and the specified path from secure
 * storage.
 *
 * @note
 *      The specified path must be an absolute path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the path doesn't exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secStoreAdmin_Delete(afb_req_t req)
{
	DBC *size_cursor;
	char delete_key_path[KEY_MAX_LEN] = "";
	int ret;
	/* get the key */
	if (get_admin_key(req, delete_key_path, 2))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
		return;
	}

	if (dbp->cursor(dbp, NULL, &size_cursor, 0))
	{
		AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_gettotalspace:Failed to init cursor");
		return;
	}
	DBT ckey;
	DBT cvalue;
	memset(&ckey, 0, sizeof(DBT));
	memset(&cvalue, 0, sizeof(DBT));

	while (!size_cursor->get(size_cursor, &ckey, &cvalue, DB_NEXT))
	{
		//The key path (to delete) must be the exactly equal to delete_key_path or, if delete_key_path is a directory, include inside.
		int cmp_path = compare_key_path(delete_key_path, DATA_STR(ckey));
		if ((cmp_path == 2) || ((cmp_path == 1) && (delete_key_path[strlen(delete_key_path) - 1] == '/')))
		{
			ret = dbp->del(dbp, NULL, &ckey, 0);
			if (ret != 0)
			{
				AFB_API_ERROR(afbBindingRoot, "can't delete key %s", DATA_STR(ckey));
				afb_req_reply_f(req, NULL, "failed", "%s", db_strerror(ret));
			}
		}
	}

	secs_sync_database();
	afb_req_reply_f(req, NULL, NULL, NULL);
}
#endif

/**
 * Gets the size, in bytes, of all items under the specified path.
 *
 * @note
 *      The specified path must be an absolute path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the path doesn't exist.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secStoreAdmin_GetSize(afb_req_t req)
{
	DBC *size_cursor;
	char kdata[KEY_MAX_LEN] = "";
	/* get the key */
	if (get_admin_key(req, kdata, 0))
	{
		AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
		return;
	}

	if (dbp->cursor(dbp, NULL, &size_cursor, 0))
	{
		AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_gettotalspace:Failed to init cursor");
		return;
	}
	DBT ckey;
	DBT cvalue;
	memset(&ckey, 0, sizeof(DBT));
	memset(&cvalue, 0, sizeof(DBT));

	long int totalsize = 0;
	while (!size_cursor->get(size_cursor, &ckey, &cvalue, DB_NEXT))
	{
		if (compare_key_path(kdata, DATA_STR(ckey)))
		{
			totalsize += (cvalue.size) * 16;
		}
	}

	struct json_object *result;
	result = json_object_new_object();
	json_object_object_add(result, "size", json_object_new_int64(totalsize));
	afb_req_reply_f(req, result, NULL, "DB gettotalspace of %s is %li", cursor_key_pass, totalsize);
}

/**
 * Gets the total space and the available free space in secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secStoreAdmin_GetTotalSpace(afb_req_t req)
{
	struct json_object *result;
	long int sz = getdbsize();
	result = json_object_new_object();
	json_object_object_add(result, "totalSize", json_object_new_int64(sz));
	json_object_object_add(result, "freeSize", json_object_new_int64(DB_MAX_SIZE - sz));

	afb_req_reply_f(req, result, NULL, "DB totalSize is %li freeSize %li", sz, DB_MAX_SIZE - sz);
}

//--------------------------------------------------------------------------------------------------

static const struct afb_auth _afb_auths_v2_monitor[] = {
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:set"},
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:get"},
	{.type = afb_auth_Or, .first = &_afb_auths_v2_monitor[1], .next = &_afb_auths_v2_monitor[0]}};

static const afb_verb_t verbs[] = {
	{.verb = "Write", .session = AFB_SESSION_NONE, .callback = secs_Write, .auth = NULL},
	{.verb = "Read", .session = AFB_SESSION_NONE, .callback = secs_Read, .auth = NULL},
	{.verb = "Delete", .session = AFB_SESSION_NONE, .callback = secs_Delete, .auth = NULL},
	{NULL}};

#ifdef ALLOW_SECS_GLOBAL
static const afb_verb_t global_verbs[] = {
	{.verb = "Write", .session = AFB_SESSION_NONE, .callback = secs_Write_global, .auth = NULL},
	{.verb = "Read", .session = AFB_SESSION_NONE, .callback = secs_Read_global, .auth = NULL},
	{.verb = "Delete", .session = AFB_SESSION_NONE, .callback = secs_Delete_global, .auth = NULL},
	{NULL}};
#endif

static const afb_verb_t admin_verbs[] = {
#ifdef ALLOW_SECS_ADMIN
	{.verb = "CreateIter", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_CreateIter, .auth = NULL},
	{.verb = "DeleteIter", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_DeleteIter, .auth = NULL},
	{.verb = "Next", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Next, .auth = NULL},
	{.verb = "GetEntry", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_GetEntry, .auth = NULL},
	{.verb = "Write", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Write, .auth = NULL},
	{.verb = "Read", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Read, .auth = NULL},
	{.verb = "CopyMetaTo", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_CopyMetaTo, .auth = NULL},
	{.verb = "Delete", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Delete, .auth = NULL},
#endif
	{.verb = "GetSize", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_GetSize, .auth = NULL},
	{.verb = "GetTotalSpace", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_GetTotalSpace, .auth = NULL},
	{NULL}};

/**
 * pre Initialize the binding.
 * @return Exit code, zero if success.
 */
static int preinit_secure_storage_binding(afb_api_t api)
{
	afb_api_t secStoreGlobal = afb_api_new_api(
		api,
		"secstoreglobal",
		"This API provides global control for secure storage",
		0,
		NULL,
		NULL);

	afb_api_set_verbs_v3(
		secStoreGlobal,
		global_verbs);

	afb_api_t secStoreAdmin = afb_api_new_api(
		api,
		"secstoreadmin",
		"This API provides administrative control for secure storage",
		0,
		NULL,
		NULL);

	afb_api_set_verbs_v3(
		secStoreAdmin,
		admin_verbs);
	return 1;
}

/**
 * Initialize the binding.
 * @return Exit code, zero if success.
 */
static int init_secure_storage_binding(afb_api_t api)
{
	int res;
	char db_path[PATH_MAX];

	res = get_db_path(db_path, sizeof db_path);
	if (res < 0 || (int)res >= (int)(sizeof db_path))
	{
		AFB_API_ERROR(afbBindingRoot, "No database filename available");
		return -1;
	}

	AFB_API_INFO(afbBindingRoot, "open database: \"%s\"", db_path);
	return open_database(db_path);
}

const afb_binding_t afbBindingExport = {
	.api = "secstorage",
	.specification = NULL,
	.verbs = verbs,
	.preinit = preinit_secure_storage_binding,
	.init = init_secure_storage_binding,
	.onevent = NULL,
	.userdata = NULL,
	.provide_class = NULL,
	.require_class = NULL,
	.require_api = NULL,
	.noconcurrency = 0};
