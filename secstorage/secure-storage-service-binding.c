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
#include <json-c/json.h>

#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>

#include "secure-storage.h"

const char *filename = "/tmp/test.json";

static void read(afb_req_t request)
{
	json_object *tmpJ;
	json_object *queryJ = afb_req_json(request);

	json_bool success = json_object_object_get_ex(queryJ, "key", &tmpJ);
	if (!success)
	{
		afb_req_fail_f(request, "ERROR", "key not found in '%s' value -> %s", json_object_get_string(queryJ), json_object_get_string(tmpJ));
		return;
	}

	const char *key = json_object_get_string(tmpJ);

	json_object *dico = json_object_from_file(filename);

	success = json_object_object_get_ex(dico, key, &tmpJ);
	if (!success)
	{
		afb_req_fail_f(request, "ERROR", "key not found in dico '%s' value -> %s", json_object_get_string(queryJ), json_object_get_string(tmpJ));
		return;
	}

	const char *value = json_object_get_string(tmpJ);

	afb_req_success_f(request, json_object_new_string(value), "SecStorage[%s] is %s", key, value);

	AFB_API_NOTICE(afbBindingV3root, "Verbosity macro at level notice invoked at read invocation %s", value);
}

static void write(afb_req_t request)
{
	json_object *tmpJ;
	json_object *queryJ = afb_req_json(request);

	json_bool success = json_object_object_get_ex(queryJ, "key", &tmpJ);
	if (!success)
	{
		afb_req_fail_f(request, "ERROR", "key not found in '%s' value -> %s", json_object_get_string(queryJ), json_object_get_string(tmpJ));
		return;
	}

	const char *key = json_object_get_string(tmpJ);

	success = json_object_object_get_ex(queryJ, "value", &tmpJ);
	if (!success)
	{
		afb_req_fail_f(request, "ERROR", "key not found in '%s' value -> %s", json_object_get_string(queryJ), json_object_get_string(tmpJ));
		return;
	}

	const char *value = json_object_get_string(tmpJ);

	json_object *dico;
	FILE *file;
	if ((file = fopen(filename, "r")) != NULL)
	{
		fclose(file);
		dico = json_object_from_file(filename);
	}
	else
	{
		dico = json_object_new_object();
	}
	json_object_object_add(dico, key, json_object_new_string(value));

	int res = json_object_to_file(filename, dico);

	afb_req_success_f(request, json_object_new_int(res), "SecStorage[%s]=%s", key, value);

	AFB_API_NOTICE(afbBindingV3root, "Verbosity macro at level notice invoked at write invocation %s", value);
}

static void delete (afb_req_t request)
{
	json_object *tmpJ;
	json_object *queryJ = afb_req_json(request);

	json_bool success = json_object_object_get_ex(queryJ, "key", &tmpJ);
	if (!success)
	{
		afb_req_fail_f(request, "ERROR", "key not found in '%s' value -> %s", json_object_get_string(queryJ), json_object_get_string(tmpJ));
		return;
	}

	char *key = json_object_get_string(tmpJ);

	json_object *dico;
	FILE *file;
	if ((file = fopen(filename, "r")) != NULL)
	{
		fclose(file);
		dico = json_object_from_file(filename);
	}
	else
	{
		afb_req_fail_f(request, "ERROR", "No file");
		return;
	}

	json_object_object_del(dico, key);

	int res = json_object_to_file(filename, dico);
	afb_req_success_f(request, json_object_new_int(res), "Delete SecStorage[%s]", key);

	AFB_API_NOTICE(afbBindingV3root, "Verbosity macro at level notice invoked at delete invocation %s", key);
	;
}

static const struct afb_auth _afb_auths_v2_monitor[] = {
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:set"},
	{.type = afb_auth_Permission, .text = "urn:AGL:permission:monitor:public:get"},
	{.type = afb_auth_Or, .first = &_afb_auths_v2_monitor[1], .next = &_afb_auths_v2_monitor[0]}};

static const afb_verb_t verbs[] = {
	{.verb = "read", .session = AFB_SESSION_NONE, .callback = read, .auth = NULL},
	{.verb = "write", .session = AFB_SESSION_NONE, .callback = write, .auth = NULL},
	{.verb = "delete", .session = AFB_SESSION_NONE, .callback = delete, .auth = NULL},
	{NULL}};

const afb_binding_t afbBindingExport = {
	.api = "secstorage",
	.specification = NULL,
	.verbs = verbs,
	.preinit = NULL,
	.init = NULL,
	.onevent = NULL,
	.userdata = NULL,
	.provide_class = NULL,
	.require_class = NULL,
	.require_api = NULL,
	.noconcurrency = 0};
