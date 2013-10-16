/**
 * chewing-sql.c
 *
 * Copyright (c) 2013
 *	libchewing Core Team. See ChangeLog for details.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file.
 */

#include "chewing-sql.h"
#include "chewing-private.h"

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

#include "sqlite3.h"
#include "plat_types.h"
#include "private.h"

const SqlStmtUserphrase SQL_STMT_USERPHRASE[STMT_USERPHRASE_COUNT] = {
	{
		"SELECT length, phrase, "
			"phone_0, phone_1, phone_2, phone_3, phone_4, phone_5, "
			"phone_6, phone_7, phone_8, phone_9, phone_10 "
			"FROM userphrase_v1",
		{ -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
	},
	{
		"SELECT time, orig_freq, max_freq, user_freq, phrase "
			"FROM userphrase_v1 WHERE length = ?5 AND "
			"phone_0 = ?10 AND phone_1 = ?11 AND phone_2 = ?12 AND "
			"phone_3 = ?13 AND phone_4 = ?14 AND phone_5 = ?15 AND "
			"phone_6 = ?16 AND phone_7 = ?17 AND phone_8 = ?18 AND "
			"phone_9 = ?19 AND phone_10 = ?20",
		{ 0, 1, 2, 3, -1, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	},
	{
		"SELECT time, orig_freq, max_freq, user_freq "
			"FROM userphrase_v1 WHERE length = ?5 AND phrase = ?6 AND "
			"phone_0 = ?10 AND phone_1 = ?11 AND phone_2 = ?12 AND "
			"phone_3 = ?13 AND phone_4 = ?14 AND phone_5 = ?15 AND "
			"phone_6 = ?16 AND phone_7 = ?17 AND phone_8 = ?18 AND "
			"phone_9 = ?19 AND phone_10 = ?20",
		{ 0, 1, 2, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	},
	{
		"INSERT OR REPLACE INTO userphrase_v1 ("
			"time, orig_freq, max_freq, user_freq, length, phrase, "
			"phone_0, phone_1, phone_2, phone_3, phone_4, phone_5, "
			"phone_6, phone_7, phone_8, phone_9, phone_10) "
			"VALUES (?1, ?2, ?3, ?4, ?5, ?6, "
			"?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20)",
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	},
	{
		"DELETE FROM userphrase_v1 WHERE length = ?5 AND phrase = ?6 AND "
			"phone_0 = ?10 AND phone_1 = ?11 AND phone_2 = ?12 AND "
			"phone_3 = ?13 AND phone_4 = ?14 AND phone_5 = ?15 AND "
			"phone_6 = ?16 AND phone_7 = ?17 AND phone_8 = ?18 AND "
			"phone_9 = ?19 AND phone_10 = ?20",
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	},
	{
		"SELECT MAX(user_freq) FROM userphrase_v1 WHERE length = ?5 AND "
			"phone_0 = ?10 AND phone_1 = ?11 AND phone_2 = ?12 AND "
			"phone_3 = ?13 AND phone_4 = ?14 AND phone_5 = ?15 AND "
			"phone_6 = ?16 AND phone_7 = ?17 AND phone_8 = ?18 AND "
			"phone_9 = ?19 AND phone_10 = ?20",
		{ -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	},
};

const SqlStmtConfig SQL_STMT_CONFIG[STMT_CONFIG_COUNT] = {
	{
		"SELECT value FROM config_v1 WHERE id = ?1",
		{ -1, 0 },
	},
	{
		"INSERT OR IGNORE INTO config_v1 (id, value) VALUES (?1, ?2)",
		{ -1, -1 },
	},
	{
		"UPDATE config_v1 SET value = value + ?2 WHERE id = ?1",
		{ -1, -1 },
	},
};

#define DB_NAME	"chewing.db"

#if defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)

#include <Shlobj.h>
#define USERPHRASE_DIR	"ChewingTextService"

static char *GetUserPhraseStoregePath(pgdata)
{
	wchar_t *tmp;
	char *path;
	int i;
	int len;

	assert(pgdata);

	len = GetEnvironmentVariableW(L"CHEWING_USER_PATH", NULL, 0);
	if (len) {
		tmp = calloc(sizeof(*tmp) * len);
		if (!tmp) {
			LOG_ERROR("calloc returns %#p", tmp);
			exit(-1);
		}

		GetEnvironmentVariableW(L"CHEWING_USER_PATH", tmp, len);

		len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tmp, -1, NULL, 0, NULL);
		++len;
		path = calloc(sizeot(*path) * len);
		if (!path) {
			free(tmp);
			LOG_ERROR("calloc returns %#p", path);
			exit(-1);
		}
		WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tmp, -1, path, len, NULL);

		free(tmp);
		return path;
	}

	len = GetEnvironmentVariableW(L"USERPROFILE", NULL, 0);
	if (len) {
		tmp = calloc(sizeof(*tmp) * len);
		if (!tmp) {
			LOG_ERROR("calloc returns %#p", tmp);
			exit(-1);
		}

		GetEnvironmentVariableW(L"USERPROFILE", tmp, len);

		len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tmp, -1, NULL, 0, NULL);
		len += 1 + strlen(USERPHRASE_DIR);
		path = calloc(sizeot(*path) * len);
		if (!path) {
			free(tmp);
			LOG_ERROR("calloc returns %#p", path);
			exit(-1);
		}
		WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tmp, -1, path, len, NULL);
		strncat(path, USERPHRASE_DIR, len);

		free(tmp);
		return path;
	}

	return NULL;
}

static int SetSQLiteTemp(ChewingData *pgdata)
{
	/*
	 * Set temporary directory is necessary for Windows platform.
	 * http://www.sqlite.org/capi3ref.html#sqlite3_temp_directory
	 */

	int ret;
	int *path;

	assert(pgdata);

	ret = GetTempPathA(0, NULL);
	path = malloc(sizeof(*path) * (ret + 1));
	if (!path) {
		LOG_ERROR("malloc returns %#p", path);
		exit(-1);
	}

	GetTempPathA(path, ret + 1);

	// FIXME: When to free sqlite3_temp_directory?
	// FIXME: thread safe?
	sqlite3_temp_directory = sqlite3_mprintf("%s", buf);
	if (sqlite3_temp_directory == 0) {
		free(path);
		LOG_ERROR("sqlite3_mprintf returns %#p", sqlite3_temp_directory);
		exit(-1);
	}

	free(path);
	return 0;
}

#else

#ifdef __MaxOSX__
/* FIXME: Shall this path pre user? */
#define USERPHRASE_DIR	"/Library/ChewingOSX"
#else
#define USERPHRASE_DIR	".chewing"
#endif

#include <string.h>
#include <unistd.h>

static char *GetUserPhraseStoregePath(ChewingData *pgdata)
{
	char *tmp;
	char *path;
	int ret;

	assert(pgdata);

	tmp = getenv("CHEWING_USER_PATH");
	if (tmp && access(tmp, W_OK) == 0) {
		ret = asprintf(&path, "%s", tmp);
		if (ret == -1) {
			LOG_ERROR("asprintf returns %d", ret);
			exit(-1);
		}
		return path;
	}

	tmp = getenv("HOME");
	if (!tmp) {
		tmp = PLAT_TMPDIR;
	}

	ret = asprintf(&path, "%s" PLAT_SEPARATOR "%s", tmp, USERPHRASE_DIR);
	if (ret == -1) {
		LOG_ERROR("asprintf returns %d", ret);
		exit(-1);
	}
	PLAT_MKDIR(path);

	return path;
}

static int SetSQLiteTemp(ChewingData *pgdata)
{
	return 0;
}

#endif


static sqlite3 *GetSQLiteInstance(ChewingData *pgdata, const char *path)
{
	int len;
	char *buf;
	int ret;
	sqlite3 *db = NULL;

	assert(pgdata);
	assert(path);

	len = strlen(path) + strlen(DB_NAME) + 1 + 1;
	buf = calloc(sizeof(*buf), len);
	if (!buf) {
		LOG_ERROR("calloc returns %#p, length = %d", buf, len);
		exit(-1);
	}

	ret = SetSQLiteTemp(pgdata);
	if (ret) {
		LOG_ERROR("SetSQLiteTemp returns %d", ret);
		goto end;
	}

	snprintf(buf, len, "%s" PLAT_SEPARATOR "%s", path, DB_NAME);
	ret = sqlite3_open(buf, &db);
	if (ret != SQLITE_OK) {
		LOG_ERROR("sqlite3_open returns %d", ret);
		goto end;
	}

end:
	free(buf);
	return db;
}


static int CreateTable(ChewingData *pgdata)
{
	int ret;

	STATIC_ASSERT(MAX_PHRASE_LEN == 11, update_database_schema_for_max_phrase_len);

	ret = sqlite3_exec(pgdata->static_data.db,
		"CREATE TABLE IF NOT EXISTS userphrase_v1 ("
		"time INTEGER,"
		"user_freq INTEGER,"
		"max_freq INTEGER,"
		"orig_freq INTEGER,"
		"length INTEGER,"
		"phone_0 INTEGER,"
		"phone_1 INTEGER,"
		"phone_2 INTEGER,"
		"phone_3 INTEGER,"
		"phone_4 INTEGER,"
		"phone_5 INTEGER,"
		"phone_6 INTEGER,"
		"phone_7 INTEGER,"
		"phone_8 INTEGER,"
		"phone_9 INTEGER,"
		"phone_10 INTEGER,"
		"phrase TEXT,"
		"PRIMARY KEY ("
			"phone_0,"
			"phone_1,"
			"phone_2,"
			"phone_3,"
			"phone_4,"
			"phone_5,"
			"phone_6,"
			"phone_7,"
			"phone_8,"
			"phone_9,"
			"phone_10,"
			"phrase)"
		")",
		NULL, NULL, NULL );
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot create table userphrase_v1, error = %d", ret);
		return -1;
	}

	ret = sqlite3_exec(pgdata->static_data.db,
		"CREATE TABLE IF NOT EXISTS config_v1 ("
		"id INTEGER,"
		"value INTEGER,"
		"PRIMARY KEY (id)"
		")",
		NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot create table config_v1, error = %d", ret);
		return -1;
	}

	return 0;
}

static int SetupUserphraseLifeTime(ChewingData *pgdata)
{
	int ret;

	ret = sqlite3_reset(pgdata->static_data.stmt_config[STMT_CONFIG_INSERT]);
	if (ret != SQLITE_OK) {
		LOG_ERROR("sqlite3_reset returns %d", ret);
		return -1;
	}

	ret = sqlite3_clear_bindings(pgdata->static_data.stmt_config[STMT_CONFIG_INSERT]);
	if (ret != SQLITE_OK) {
		LOG_ERROR("sqlite3_clear_bindings returns %d", ret);
		return -1;
	}

	ret = sqlite3_bind_int(pgdata->static_data.stmt_config[STMT_CONFIG_INSERT],
		BIND_CONFIG_ID, CONFIG_ID_LIFETIME);
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot bind ?%d to %d in stmt %s, error = %d",
			BIND_CONFIG_ID, CONFIG_ID_LIFETIME,
			SQL_STMT_CONFIG[STMT_CONFIG_INSERT].stmt, ret);
		return -1;
	}

	ret = sqlite3_bind_int(pgdata->static_data.stmt_config[STMT_CONFIG_INSERT],
		BIND_CONFIG_VALUE, 0);
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot bind ?%d to %d in stmt %s, error = %d",
			BIND_CONFIG_VALUE, 0,
			SQL_STMT_CONFIG[STMT_CONFIG_INSERT].stmt, ret);
		return -1;
	}

	ret = sqlite3_step(pgdata->static_data.stmt_config[STMT_CONFIG_INSERT]);
	if (ret != SQLITE_DONE) {
		LOG_ERROR("sqlite3_step returns %d", ret);
		return -1;
	}


	ret = sqlite3_reset(pgdata->static_data.stmt_config[STMT_CONFIG_SELECT]);
	if (ret != SQLITE_OK) {
		LOG_ERROR("sqlite3_reset returns %d", ret);
		return -1;
	}

	ret = sqlite3_clear_bindings(pgdata->static_data.stmt_config[STMT_CONFIG_SELECT]);
	if (ret != SQLITE_OK) {
		LOG_ERROR("sqlite3_clear_bindings returns %d", ret);
		return -1;
	}

	ret = sqlite3_bind_int(pgdata->static_data.stmt_config[STMT_CONFIG_SELECT],
		BIND_CONFIG_ID, CONFIG_ID_LIFETIME);
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot bind ?%d to %d in stmt %s, error = %d",
			BIND_CONFIG_ID, CONFIG_ID_LIFETIME,
			SQL_STMT_CONFIG[STMT_CONFIG_SELECT].stmt, ret);
		return -1;
	}

	ret = sqlite3_step(pgdata->static_data.stmt_config[STMT_CONFIG_SELECT]);
	if (ret != SQLITE_ROW) {
		LOG_ERROR("sqlite3_step returns %d", ret);
		return -1;
	}

	pgdata->static_data.original_lifetime = sqlite3_column_int(
		pgdata->static_data.stmt_config[STMT_CONFIG_SELECT],
		SQL_STMT_CONFIG[STMT_CONFIG_SELECT].column[COLUMN_CONFIG_VALUE]);
	pgdata->static_data.new_lifetime = pgdata->static_data.original_lifetime;

	return 0;
}

static int UpdateLifeTime(ChewingData *pgdata)
{
	int ret;

	ret = sqlite3_reset(pgdata->static_data.stmt_config[STMT_CONFIG_INCREASE]);
	if (ret != SQLITE_OK) {
		LOG_ERROR("sqlite3_reset returns %d", ret);
		return -1;
	}

	ret = sqlite3_clear_bindings(pgdata->static_data.stmt_config[STMT_CONFIG_INCREASE]);
	if (ret != SQLITE_OK) {
		LOG_ERROR("sqlite3_clear_bindings returns %d", ret);
		return -1;
	}

	ret = sqlite3_bind_int(pgdata->static_data.stmt_config[STMT_CONFIG_INCREASE],
		BIND_CONFIG_ID, CONFIG_ID_LIFETIME);
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot bind ?%d to %d in stmt %s, error = %d",
			BIND_CONFIG_ID, CONFIG_ID_LIFETIME,
			SQL_STMT_CONFIG[STMT_CONFIG_INCREASE].stmt, ret);
		return -1;
	}

	ret = sqlite3_bind_int(pgdata->static_data.stmt_config[STMT_CONFIG_INCREASE],
		BIND_CONFIG_VALUE,
		pgdata->static_data.new_lifetime - pgdata->static_data.original_lifetime);
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot bind ?%d to %d in stmt %s, error = %d",
			BIND_CONFIG_VALUE,
			pgdata->static_data.new_lifetime - pgdata->static_data.original_lifetime,
			SQL_STMT_CONFIG[STMT_CONFIG_INCREASE].stmt, ret);
		return -1;
	}

	ret = sqlite3_step(pgdata->static_data.stmt_config[STMT_CONFIG_INCREASE]);
	if (ret != SQLITE_DONE) {
		LOG_ERROR("sqlite3_step returns %d", ret);
		return ret;
	}

	return 0;
}

static int ConfigDatabase(ChewingData *pgdata)
{
	int ret;

	assert(pgdata);
	assert(pgdata->static_data.db);

	ret = sqlite3_exec(pgdata->static_data.db, "PRAGMA synchronous=OFF", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		LOG_ERROR("Cannot set synchronous=OFF, error = %d", ret);
		return -1;
	}

	return 0;
}

static int CreateStmt(ChewingData *pgdata)
{
	int i;
	int ret;

	assert(pgdata);

	STATIC_ASSERT(ARRAY_SIZE(SQL_STMT_CONFIG) == ARRAY_SIZE(pgdata->static_data.stmt_config),
		stmt_config_size_mismatch);
	STATIC_ASSERT(ARRAY_SIZE(SQL_STMT_USERPHRASE) == ARRAY_SIZE(pgdata->static_data.stmt_userphrase),
		stmt_userphrase_size_mismatch);

	for (i = 0; i < ARRAY_SIZE(SQL_STMT_CONFIG); ++i) {
		ret = sqlite3_prepare_v2(pgdata->static_data.db,
			SQL_STMT_CONFIG[i].stmt, -1,
			&pgdata->static_data.stmt_config[i], NULL);
		if (ret != SQLITE_OK) {
			LOG_ERROR("Cannot create stmt %s", SQL_STMT_CONFIG[i].stmt);
			return -1;
		}
	}

	for (i = 0; i < ARRAY_SIZE(SQL_STMT_USERPHRASE); ++i) {
		ret = sqlite3_prepare_v2(pgdata->static_data.db,
			SQL_STMT_USERPHRASE[i].stmt, -1,
			&pgdata->static_data.stmt_userphrase[i], NULL);
		if (ret != SQLITE_OK) {
			LOG_ERROR("Cannot create stmt %s", SQL_STMT_USERPHRASE[i].stmt);
			return -1;
		}
	}

	return 0;
}

int InitSql(ChewingData *pgdata)
{
	int ret;
	char *path = NULL;

	assert(!pgdata->static_data.db);

	path = GetUserPhraseStoregePath(pgdata);
	if (!path) {
		LOG_ERROR("GetUserPhraseStoregePath returns %#p", path);
		goto error;
	}

	pgdata->static_data.db = GetSQLiteInstance(pgdata, path);
	if (!pgdata->static_data.db) {
		LOG_ERROR("GetSQLiteInstance fails");
		goto error;
	}

	ret = ConfigDatabase(pgdata);
	if (ret) {
		LOG_ERROR("ConfigDatabase returns %d", ret);
		goto error;
	}

	ret = CreateTable(pgdata);
	if (ret) {
		LOG_ERROR("CreateTable returns %d", ret);
		goto error;
	}

	ret = CreateStmt(pgdata);
	if (ret) {
		LOG_ERROR("CreateStmt returns %d", ret);
		goto error;
	}

	ret = SetupUserphraseLifeTime(pgdata);
	if (ret) {
		LOG_ERROR("SetupUserphraseLiftTime returns %d", ret);
		goto error;
	}

	/* FIXME: Migrate uhash.dat */
	/* FIXME: Normalize lifttime when necessary. */

	free(path);
	return 0;

error:
	TerminateSql(pgdata);
	free(path);
	return -1;
}

void TerminateSql(ChewingData *pgdata)
{
	int i;
	int ret;

	UpdateLifeTime(pgdata);

	for (i = 0; i < ARRAY_SIZE(pgdata->static_data.stmt_config); ++i) {
		sqlite3_finalize(pgdata->static_data.stmt_config[i]);
		pgdata->static_data.stmt_config[i] = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(pgdata->static_data.stmt_userphrase); ++i) {
		sqlite3_finalize(pgdata->static_data.stmt_userphrase[i]);
		pgdata->static_data.stmt_userphrase[i] = NULL;
	}

	ret = sqlite3_close(pgdata->static_data.db);
	assert(SQLITE_OK == ret);
	pgdata->static_data.db = NULL;
}
