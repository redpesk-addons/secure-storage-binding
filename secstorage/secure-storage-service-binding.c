/*
 * Copyright (C) 2016-2020 IoT.bzh Company
 *
 * $RP_BEGIN_LICENSE$
 * Commercial License Usage
 *  Licensees holding valid commercial IoT.bzh licenses may use this file in
 *  accordance with the commercial license agreement provided with the
 *  Software or, alternatively, in accordance with the terms contained in
 *  a written agreement between you and The IoT.bzh Company. For licensing terms
 *  and conditions see https://www.iot.bzh/terms-conditions. For further
 *  information use the contact form at https://www.iot.bzh/contact.
 *
 * GNU General Public License Usage
 *  Alternatively, this file may be used under the terms of the GNU General
 *  Public license version 3. This license is as published by the Free Software
 *  Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
 *  of this file. Please review the following information to ensure the GNU
 *  General Public License requirements will be met
 *  https://www.gnu.org/licenses/gpl-3.0.html.
 * $RP_END_LICENSE$
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define AFB_BINDING_VERSION 4
#include <afb-helpers4/afb-req-utils.h>
#include <afb/afb-binding.h>

#include <db.h>

#define DB_FILE        "secstorage.db" /* DB On-disk file name*/
#define DB_PASSWD_FILE "test.passwd"   /* DB PASSWD On-disk file name*/
static DB *dbp;
/* DB structure handle */

#define DATA_PTR(k) ((void *)((k).data))
#define DATA_STR(k) ((char *)((k).data))

#define VALUE_MAX_LEN 8192
#define KEY_MAX_LEN   255
#define DB_MAX_SIZE   16777216

#define DATA_SET(k, d, s)            \
    do {                             \
        memset((k), 0, sizeof *(k)); \
        (k)->data = (void *)d;       \
        (k)->size = (uint32_t)s;     \
    } while (0)

#ifdef SECSTOREADMIN
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
    char *afb_passwd_dir;
    char db_passwd_path[PATH_MAX] = "";
    int res;

    afb_passwd_dir = secure_getenv("AFB_PASSWD_DIR");
    if (afb_passwd_dir) {
        res = snprintf(db_passwd_path, sizeof db_passwd_path, "%s/%s", afb_passwd_dir,
                       DB_PASSWD_FILE);
    }
    else {
        AFB_API_ERROR(afbBindingRoot, "Failed to find var env AFB_PASSWD_DIR");
        return -1;
    }

    fp = fopen(db_passwd_path, "r");

    // test for files not existing.
    if (fp == NULL) {
        AFB_API_ERROR(afbBindingRoot, "Failed to open DB file passwd: %s", db_passwd_path);
        return -1;
    }

    res = fscanf(fp, "%s", db_passwd);

    if (res != 1) {
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

    if (res != 0) {
        AFB_API_ERROR(afbBindingRoot, "Failed database creation: %s.", db_strerror(res));
        return -1;
    }
    /*
    Get the password to encrypt the database.
    */
    int res_pwd = get_passwd(db_passwd);
    if (res_pwd != 0) {
        AFB_API_ERROR(afbBindingRoot, "Failed to get database password.");
        return -1;
    }
    dbp->set_encrypt(
        dbp,             /* DB structure pointer */
        db_passwd,       /*Passworld to  encryption/decryption of the database*/
        DB_ENCRYPT_AES); /*Use the Rijndael/AES algorithm for encryption or decryption.*/

    res = dbp->open(dbp,       /* DB structure pointer */
                    NULL,      /* Transaction pointer */
                    path,      /* On-disk file that holds the database. */
                    NULL,      /* Optional logical database name */
                    DB_BTREE,  /* Database access method */
                    DB_CREATE, /* Open flags */
                    0600);     /* File mode */

    if (res != 0) {
        AFB_API_ERROR(afbBindingRoot, "Failed to open the '%s' database: %s.", path,
                      db_strerror(res));
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
    if (afb_workdir) {
        res = snprintf(buffer, size, "%s/%s", afb_workdir, DB_FILE);
    }
    else {
        AFB_API_WARNING(afbBindingRoot, "Failed to find AFB_WORKDIR");
        local_workdir = secure_getenv("PWD");
        if (local_workdir)
            res = snprintf(buffer, size, "%s/%s", local_workdir, DB_FILE);
        else {
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
    afb_data_t afb_arg;
    const struct json_object *arg;
    struct json_object *item;

    // convert parameter 0 to a JSON object
    if (afb_req_param_convert(req, 0, AFB_PREDEFINED_TYPE_JSON_C, &afb_arg) < 0) {
        afb_req_reply(req, AFB_ERRNO_INVALID_REQUEST, 0, NULL);
        return -1;
    }

    // retrieves the pointer to the JSON object
    arg = afb_data_ro_pointer(afb_arg);
    if (!arg || json_object_get_type(arg) != json_type_object) {
        afb_req_reply(req, AFB_ERRNO_INVALID_REQUEST, 0, NULL);
        return -1;
    }
    // retrieves the "key" field
    if (!json_object_object_get_ex(arg, "key", &item)) {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "no key");
        return -1;
    }
    // convert the key value to string
    *req_key = json_object_get_string(item);
    if (!item || json_object_get_type(item) != json_type_string || !(*req_key) ||
        strlen(*req_key) == 0) {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "bad-key");
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
    afb_data_t args;

    struct json_object *arg;
    struct json_object *item;
    /* get the key of the req*/
    afb_req_param_convert(req, 0, AFB_PREDEFINED_TYPE_JSON, &args);
    arg = afb_data_ro_pointer(args);
    if (!json_object_object_get_ex(arg, "path", &item)) {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "no path");
        return -1;
    }
    /* Convert the req path value to string*/
    if (!item || !(req_key = json_object_get_string(item)) || !(lreq_key = strlen(req_key))) {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "bad path");
        return -1;
    }
    strcpy(path, req_key);
    return 0;
}

/**
 * check_path_end:
 *	0: The path must end with a "/"
 *	1: The path must not end with a "/"
 *	2: Do not Check the end of the path.
 */
static int get_admin_key(afb_req_t req, char *data, int check_path_end)
{
    const char *req_key = NULL;
    if (get_rawkey(req, &req_key)) {
        return -1;
    }

    if (req_key[0] != '/') {
        AFB_API_ERROR(afbBindingRoot, "Admin path requested must start with \"%s\"", "/");
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "Admin path must be absolute");
        return -1;
    }

    if ((check_path_end != 2) && !check_path_end ^ (req_key[strlen(req_key) - 1] == '/')) {
        if (check_path_end) {  // The key name can contain path separators, '/', to
            //* help organize an app's data.  Key names cannot end with a trailing separator.
            AFB_API_ERROR(afbBindingRoot, "Forbidden use of char \"%s\" at the end of key request",
                          "/");
            afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST,
                                 "Forbidden char at the end of key request");
            return -1;
        }
        else {
            AFB_API_ERROR(afbBindingRoot, "Path requested must end with \"%s\"", "/");
            afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "Forbiden Path format");
            return -1;
        }
    }
    strcpy(data, req_key);
    return 0;
}

/**
 * Returns the database key for the 'req'
 */
static int get_key(afb_req_t req, char *data, int global)
{
    char *appid;
    const char *req_key;

    if (get_rawkey(req, &req_key)) {
        return -1;
    }

    if (req_key[strlen(req_key) - 1] == '/') {
        AFB_API_ERROR(afbBindingRoot, "Forbidden use of char \"%s\" at the end of key request",
                      "/");
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST,
                             "Forbiden char at the end of key request");
        return -1;
    }

    if (global) {
        appid = GLOBAL_PATH;
    }
    else {
        /* get the appid */
        struct json_object *client_info = afb_req_get_client_info(req);
        const char *client_id = "default-client";  // Default value

        if (client_info) {
            struct json_object *jappid;
            if (json_object_object_get_ex(client_info, "id", &jappid)) {
                client_id = json_object_get_string(jappid);
            }
            else {
                AFB_API_NOTICE(afbBindingRoot,
                               "Client ID not found in client_info: using default ID");
            }
            json_object_put(client_info);
        }
        else {
            AFB_API_NOTICE(afbBindingRoot, "Client info NULL: using default ID");
        }

        appid = strdup(client_id);
        if (!appid) {
            AFB_API_ERROR(afbBindingRoot, "Failed to allocate memory for appid");
            return -1;
        }

        if (!strcat(data, "/")) {
            afb_req_reply_string(req, AFB_ERRNO_INTERNAL_ERROR,
                                 "Failed to concatenate '/' to the key");
            free(appid);
            return -1;
        }
    }

    if (!strcat(data, appid)) {
        afb_req_reply_string(req, AFB_ERRNO_INTERNAL_ERROR,
                             "key generation from application id failed");
        free(appid);
        return -1;
    }

    if (!global)
        free(appid);

    if (req_key[0] != '/' && !strcat(data, "/")) {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "key generation failed");
        return -1;
    }

    if (!strcat(data, req_key)) {
        afb_req_reply(req, AFB_ERRNO_INVALID_REQUEST, 0, NULL);
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "key generation from json key failed");
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
    afb_data_t args;

    const struct json_object *arg;
    struct json_object *item;

    /* get the value */
    if (afb_req_param_convert(req, 0, AFB_PREDEFINED_TYPE_JSON_C, &args) < 0) {
        afb_req_reply(req, AFB_ERRNO_INVALID_REQUEST, 0, NULL);
        return -1;
    }

    arg = afb_data_ro_pointer(args);
    if (!json_object_object_get_ex(arg, "value", &item)) {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "no-value");
        return -1;
    }
    value = json_object_get_string(item);
    if (!value) {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "out of memory");
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

    if (res == 0) {
        result = json_object_new_object();
        val = json_tokener_parse(DATA_STR(data));
        json_object_object_add(result, "value", val ? val : json_object_new_string(DATA_STR(data)));

        const char *json_str = json_object_to_json_string(result);
        size_t json_len = strlen(json_str);

        afb_data_t response;
        afb_create_data_copy(&response, AFB_PREDEFINED_TYPE_JSON, json_str, json_len + 1);
        afb_req_reply(req, 0, 1, &response);
    }
    else {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "Failed to get value");
    }
}

/**
 * API write
 */
static void p_secs_raw_write(afb_req_t req, DBT *key, DBT *data)
{
    AFB_API_DEBUG(afbBindingRoot, "put: key=\"%s\", value=\"%s\"", DATA_STR(*key), DATA_STR(*data));

    int res = dbp->put(dbp, NULL, key, data, DB_OVERWRITE_DUP);

    if (res == 0) {
        afb_req_reply(req, 0, 0, NULL);
    }
    else {
        AFB_API_ERROR(afbBindingRoot, "Failed to insert %s with %s", DATA_STR(*key),
                      DATA_STR(*data));
        afb_req_reply_string(req, AFB_ERRNO_INTERNAL_ERROR, "Failed to insert");
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
    if (ret == 0) {
        afb_req_reply(req, 0, 0, NULL);
    }
    else {
        AFB_API_ERROR(afbBindingRoot, "can't delete key %s", DATA_STR(*key));
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "Failed to delete");
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
    if (get_key(req, kdata, global)) {
        AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
        return;
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
    if (get_key(req, kdata, global)) {
        AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req key parameter");
        return;
    }

    /* get the value */
    if (get_value(req, &value)) {
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

    if (get_key(req, kdata, global)) {
        AFB_API_ERROR(afbBindingRoot, "secs_delete:Failed to get req key parameter");
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
static void write_verb(afb_req_t req, unsigned nparams, afb_data_t const *params)
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
static void read_verb(afb_req_t req, unsigned nparams, afb_data_t const *params)
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
static void delete_verb(afb_req_t req, unsigned nparams, afb_data_t const *params)
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
static void secs_Write_global(afb_req_t req, unsigned nparams, afb_data_t const *params)
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
static void secs_Read_global(afb_req_t req, unsigned nparams, afb_data_t const *params)
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
static void secs_Delete_global(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    p_secs_delete(req, 1);
}
//--------------------------------------------------------------------------------------------------
#endif

long int getdbsize()
{
    char db_path[PATH_MAX] = "";

    int res = get_db_path(db_path, sizeof db_path);
    if (res < 0 || (int)res >= (int)(sizeof db_path)) {
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
    while (*first_key == *second_second) {
        if (*first_key == '\0' || *second_second == '\0')
            break;

        first_key++;
        second_second++;
    }

    if (*first_key == '\0') {
        if (*second_second == '\0') {
            return 2;
        }
        else {
            return 1;
        }
    }
    else {
        return 0;
    }
}

// --------------------------------------------------------------------------------------------------
#ifdef SECSTOREADMIN
static int copy_db_file(const char *from, const char *to)
{
    AFB_API_NOTICE(afbBindingRoot, "copy %s to %s.", from, to);
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) {
        AFB_API_NOTICE(afbBindingRoot, "Open %s failed", from);
        return -1;
    }

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0) {
        goto out_error;
    }

    AFB_API_NOTICE(afbBindingRoot, "Start copy");
    while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;
        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR) {
                goto out_error;
            }
        } while (nread > 0);
    }

    AFB_API_DEBUG(afbBindingRoot, "End copy");
    if (nread == 0) {
        if (close(fd_to) < 0) {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

out_error:
    AFB_API_ERROR(afbBindingRoot, "copy %s to %s.", from, to);
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
static void secStoreAdmin_CreateIter(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    /*If cursor is set to a old value close it*/
    if (cursor) {
        cursor->close(cursor);
    }
    /* cursor_key_pass must be set at least at "/" */
    if (get_admin_key(req, cursor_key_pass, 0)) {
        AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
        return;
    }

    if (dbp->cursor(dbp, NULL, &cursor, 0)) {
        AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_createiter:Failed to init cursor");
        return;
    }
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "iterator", json_object_new_int64(1));

    const char *json_str = json_object_to_json_string(result);
    size_t json_len = strlen(json_str);

    afb_data_t response;
    afb_create_data_copy(&response, AFB_PREDEFINED_TYPE_JSON, json_str, json_len + 1);

    afb_req_reply(req, 0, 1, &response);
}

/**
 * Deletes an iterator.
 */
static void secStoreAdmin_DeleteIter(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    if (cursor->del(cursor, 0)) {
        AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_deleteIter:Failed to delete cursor");
        return;
    }
    else {
        afb_req_reply(req, 0, 0, NULL);
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
static void secStoreAdmin_Next(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    DBT ckey;
    DBT cvalue;
    memset(&ckey, 0, sizeof(DBT));
    memset(&cvalue, 0, sizeof(DBT));

    while (!cursor->get(cursor, &ckey, &cvalue, DB_NEXT)) {
        if (compare_key_path(cursor_key_pass, DATA_STR(ckey)) > 0) {
            break;
        }
    }
    afb_req_reply(req, 0, 0, NULL);
}

/**
 * Get the current entry's name.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entry name.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 */
static void secStoreAdmin_GetEntry(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    DBT ckey;
    DBT cvalue;
    int res;
    struct json_object *result;
    struct json_object *val;

    memset(&ckey, 0, sizeof(DBT));
    memset(&cvalue, 0, sizeof(DBT));
    res = cursor->get(cursor, &ckey, &cvalue, DB_CURRENT);

    if (res == 0) {
        result = json_object_new_object();
        val = json_tokener_parse(DATA_STR(cvalue));
        json_object_object_add(result, "value", val ? val : json_object_new_string(DATA_STR(ckey)));

        const char *json_str = json_object_to_json_string(result);
        size_t json_len = strlen(json_str);

        afb_data_t response;
        afb_create_data_copy(&response, AFB_PREDEFINED_TYPE_JSON, json_str, json_len + 1);

        afb_req_reply(req, 0, 1, &response);
    }
    else {
        afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "Failed to read datas");
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
static void secStoreAdmin_Write(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    DBT key;
    DBT value;
    char kdata[KEY_MAX_LEN] = "";

    memset(&value, 0, sizeof(DBT));

    /* get the key */
    if (get_admin_key(req, kdata, 1)) {
        AFB_API_ERROR(afbBindingRoot, "secs_write:Failed to get req key parameter");
        return;
    }
    /* get the value */
    if (get_value(req, &value)) {
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
static void secStoreAdmin_Read(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    DBT key;
    char kdata[KEY_MAX_LEN] = "";

    memset(&key, 0, sizeof(DBT));
    memset(&key, 0, sizeof(DBT));
    if (get_admin_key(req, kdata, 1)) {
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
static void secStoreAdmin_CopyMetaTo(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    secs_sync_database();
    char db_path[PATH_MAX] = "";
    char db_path_dest[KEY_MAX_LEN] = "";

    if (get_path(req, db_path_dest)) {
        return;
    }
    int res = get_db_path(db_path, sizeof db_path);
    if (res < 0 || (int)res >= (int)(sizeof db_path)) {
        return;
    };

    copy_db_file(db_path, db_path_dest);
    afb_req_reply(req, 0, 0, NULL);
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
static void secStoreAdmin_Delete(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    DBC *size_cursor;
    char delete_key_path[KEY_MAX_LEN] = "";
    int ret;
    /* get the key */
    if (get_admin_key(req, delete_key_path, 2)) {
        AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
        return;
    }

    if (dbp->cursor(dbp, NULL, &size_cursor, 0)) {
        AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_gettotalspace:Failed to init cursor");
        return;
    }
    DBT ckey;
    DBT cvalue;
    memset(&ckey, 0, sizeof(DBT));
    memset(&cvalue, 0, sizeof(DBT));

    while (!size_cursor->get(size_cursor, &ckey, &cvalue, DB_NEXT)) {
        // The key path (to delete) must be the exactly equal to delete_key_path or, if
        // delete_key_path is a directory, include inside.
        int cmp_path = compare_key_path(delete_key_path, DATA_STR(ckey));
        if ((cmp_path == 2) ||
            ((cmp_path == 1) && (delete_key_path[strlen(delete_key_path) - 1] == '/'))) {
            ret = dbp->del(dbp, NULL, &ckey, 0);
            if (ret != 0) {
                AFB_API_ERROR(afbBindingRoot, "can't delete key %s", DATA_STR(ckey));
                afb_req_reply_string(req, AFB_ERRNO_INVALID_REQUEST, "Failed");
            }
        }
    }

    secs_sync_database();
    afb_req_reply(req, 0, 0, NULL);
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
static void secStoreAdmin_GetSize(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    DBC *size_cursor;
    char cursor_key[KEY_MAX_LEN] = "";
    /* get the key */
    if (get_admin_key(req, cursor_key, 0)) {
        AFB_API_ERROR(afbBindingRoot, "secs_read:Failed to get req key parameter");
        return;
    }

    if (dbp->cursor(dbp, NULL, &size_cursor, 0)) {
        AFB_API_ERROR(afbBindingRoot, "secStoreAdmin_gettotalspace:Failed to init cursor");
        return;
    }
    DBT ckey;
    DBT cvalue;
    memset(&ckey, 0, sizeof(DBT));
    memset(&cvalue, 0, sizeof(DBT));

    long int totalsize = 0;
    while (!size_cursor->get(size_cursor, &ckey, &cvalue, DB_NEXT)) {
        if (compare_key_path(cursor_key, DATA_STR(ckey))) {
            totalsize += (cvalue.size) * 16;
        }
    }

    struct json_object *result;
    result = json_object_new_object();
    json_object_object_add(result, "size", json_object_new_int64(totalsize));

    const char *json_str = json_object_to_json_string(result);
    size_t json_len = strlen(json_str);

    afb_data_t response;
    afb_create_data_copy(&response, AFB_PREDEFINED_TYPE_JSON, json_str, json_len + 1);

    afb_req_reply(req, 0, 1, &response);
}

/**
 * Gets the total space and the available free space in secure storage.
 *
 * @return
 *      LE_OK if successful.
 *      LE_UNAVAILABLE if the secure storage is currently unavailable.
 *      LE_FAULT if there was some other error.
 */
static void secStoreAdmin_GetTotalSpace(afb_req_t req, unsigned nparams, afb_data_t const *params)
{
    struct json_object *result;
    long int sz = getdbsize();
    result = json_object_new_object();
    json_object_object_add(result, "totalSize", json_object_new_int64(sz));
    json_object_object_add(result, "freeSize", json_object_new_int64(DB_MAX_SIZE - sz));

    const char *json_str = json_object_to_json_string(result);
    size_t json_len = strlen(json_str);

    afb_data_t response;
    afb_create_data_copy(&response, AFB_PREDEFINED_TYPE_JSON, json_str, json_len + 1);

    afb_req_reply(req, 0, 1, &response);
}

//--------------------------------------------------------------------------------------------------

static const struct afb_auth _afb_auths_v2_monitor[] = {
    {.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:set"},
    {.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:get"},
    {.type = afb_auth_Or, .first = &_afb_auths_v2_monitor[1], .next = &_afb_auths_v2_monitor[0]}};

static const afb_verb_t verbs[] = {
    {.verb = "Write", .session = AFB_SESSION_NONE, .callback = write_verb, .auth = NULL},
    {.verb = "Read", .session = AFB_SESSION_NONE, .callback = read_verb, .auth = NULL},
    {.verb = "Delete", .session = AFB_SESSION_NONE, .callback = delete_verb, .auth = NULL},
    {NULL}};

#ifdef ALLOW_SECS_GLOBAL
static const afb_verb_t global_verbs[] = {
    {.verb = "Write", .session = AFB_SESSION_NONE, .callback = secs_Write_global, .auth = NULL},
    {.verb = "Read", .session = AFB_SESSION_NONE, .callback = secs_Read_global, .auth = NULL},
    {.verb = "Delete", .session = AFB_SESSION_NONE, .callback = secs_Delete_global, .auth = NULL},
    {NULL}};
#endif

static const afb_verb_t admin_verbs[] = {
#ifdef SECSTOREADMIN
    {.verb = "CreateIter",
     .session = AFB_SESSION_NONE,
     .callback = secStoreAdmin_CreateIter,
     .auth = NULL},
    {.verb = "DeleteIter",
     .session = AFB_SESSION_NONE,
     .callback = secStoreAdmin_DeleteIter,
     .auth = NULL},
    {.verb = "Next", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Next, .auth = NULL},
    {.verb = "GetEntry",
     .session = AFB_SESSION_NONE,
     .callback = secStoreAdmin_GetEntry,
     .auth = NULL},
    {.verb = "Write", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Write, .auth = NULL},
    {.verb = "Read", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Read, .auth = NULL},
    {.verb = "CopyMetaTo",
     .session = AFB_SESSION_NONE,
     .callback = secStoreAdmin_CopyMetaTo,
     .auth = NULL},
    {.verb = "Delete", .session = AFB_SESSION_NONE, .callback = secStoreAdmin_Delete, .auth = NULL},
#endif
    {.verb = "GetSize",
     .session = AFB_SESSION_NONE,
     .callback = secStoreAdmin_GetSize,
     .auth = NULL},
    {.verb = "GetTotalSpace",
     .session = AFB_SESSION_NONE,
     .callback = secStoreAdmin_GetTotalSpace,
     .auth = NULL},
    {NULL}};

/**
 * pre Initialize the binding.
 * @return Exit code, zero if success.
 */
static int preinit_secure_storage_binding(afb_api_t api)
{
    afb_api_t secStoreAdmin = NULL;
    afb_api_t secStoreGlobal = NULL;

    int rc0 = afb_create_api(&secStoreAdmin, "secstoreadmin",
                             "This API provides administrative control for secure storage", 0, NULL,
                             NULL);

    if (rc0 < 0) {
        AFB_API_ERROR(afbBindingRoot, "Failed to create 'secstoreadmin' API");
        return -1;
    }

    afb_api_set_verbs(secStoreAdmin, admin_verbs);

    int rc1 = afb_create_api(&secStoreGlobal, "secstoreglobal", "Global API", 0, NULL, NULL);

    if (rc1 < 0) {
        AFB_API_ERROR(afbBindingRoot, "Failed to create 'secstoreadmin' API");
        return -1;
    }

    afb_api_set_verbs(secStoreGlobal, global_verbs);

    return 0;
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
    if (res < 0 || (int)res >= (int)(sizeof db_path)) {
        AFB_API_ERROR(afbBindingRoot, "No database filename available");
        return -1;
    }

    AFB_API_INFO(afbBindingRoot, "open database: \"%s\"", db_path);
    return open_database(db_path);
}

/**
 * @brief Binding Callback
 *
 * @param api       the api that receive the callback
 * @param ctlid     identifier of the reason of the call (@see afb_ctlid)
 * @param ctlarg    data associated to the call
 * @param userdata  the userdata of the api (@see afb_api_get_userdata)
 */
int binding_ctl(afb_api_t api, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *userdata)
{
    switch (ctlid) {
    case afb_ctlid_Pre_Init:
        if (preinit_secure_storage_binding(api) < 0)
            return -1;
        afb_api_seal(api);
        break;

    case afb_ctlid_Init:
        AFB_API_NOTICE(api, "Initialization");
        if (init_secure_storage_binding(api) < 0) {
            AFB_API_ERROR(api, "Failed during Initialization");
            return -1;
        }
        AFB_API_NOTICE(api, "Initialization finished");
        break;

    default:
        break;
    }
    return 0;
}

const afb_binding_t afbBindingExport = {.api = "secstorage",
                                        .specification = NULL,
                                        .verbs = verbs,
                                        .mainctl = binding_ctl,
                                        .userdata = NULL,
                                        .provide_class = NULL,
                                        .require_class = NULL,
                                        .require_api = NULL,
                                        .noconcurrency = 0};
