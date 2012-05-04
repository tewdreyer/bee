#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "sqlite3.h"
#include "bee_db_query.h"

#define BEE_DB_DB_NAME             "bee.db"

#define EXIT_BEE_DB_OK             0
#define EXIT_BEE_DB_MISSUSE        1
#define EXIT_BEE_DB_REBUILD_FAILED 2

#define BEE_DB_ACTION_UNKNOWN      0
#define BEE_DB_ACTION_REBUILD      1

#define TYPE_FILE                  0
#define TYPE_PACKAGE               1

static char *bee_version(void)
{
    static char *bee_v = NULL;

    if (!bee_v)
        bee_v = getenv("BEE_VERSION");

    return bee_v;
}

static char *bee_metadir(void)
{
    static char *bee_md = NULL;

    if (!bee_md)
        bee_md = getenv("BEE_METADIR");

    return bee_md;
}

static char *bee_cachedir(void)
{
    static char *bee_cd = NULL;

    if (!bee_cd)
        bee_cd = getenv("BEE_CACHEDIR");

    return bee_cd;
}

static void check_env(void)
{
    if (!bee_version()) {
        fprintf(stderr, "BEE-ERROR: please call bee-dep from bee\n");
        exit(EXIT_FAILURE);
    }

    if (!bee_metadir()) {
        fprintf(stderr, "BEE-ERROR: BEE_METADIR not set\n");
        exit(EXIT_FAILURE);
    }

    if (!bee_cachedir()) {
        fprintf(stderr, "BEE_ERROR: BEE_CACHEDIR not set\n");
        exit(EXIT_FAILURE);
    }
}

static char *bee_db_db_filename(void)
{
    static char db[PATH_MAX + 1] = {0};

    if (!db[0])
        sprintf(db, "%s/%s", bee_cachedir(), BEE_DB_DB_NAME);

    return db;
}

static int parse_action(char *action)
{
    if(strcmp("rebuild", action) == 0)
        return BEE_DB_ACTION_REBUILD;

    return BEE_DB_ACTION_UNKNOWN;
}

int bee_db_perform_query(sqlite3 *db_handle, const char *query, void (*f_row_action)(void))
{
    int ret = 0;
    const char *tail = NULL;
    sqlite3_stmt *statement;

#ifdef DEBUG
    printf("%s\n%s\n%s\n", db_handle, query, f_row_action);
#endif

    ret = sqlite3_prepare_v2(db_handle, query, strlen(query), &statement, &tail);
    if(ret != SQLITE_OK) {
        fprintf(stderr, "failed to prepare query: %s\n", sqlite3_errmsg(db_handle));
        return 0;
    }

    while((ret = sqlite3_step(statement)) != SQLITE_DONE) {
        if(ret != SQLITE_ROW) {
            fprintf(stderr, "failed to retrieve row: %s\n", sqlite3_errmsg(db_handle));
            return 0;
        }

        f_row_action();
    }

    ret = sqlite3_finalize(statement);
    if(ret != SQLITE_OK) {
        fprintf(stderr, "failed to destroy query: %s\n", sqlite3_errmsg(db_handle));
        return 0;
    }

    return 1;
}

int bee_db_db_remove(void)
{
    if(remove(bee_db_db_filename()) != 0 && errno != ENOENT) {
        fprintf(stderr, "failed to remove %s: %s", bee_db_db_filename(), strerror(errno));
        return 0;
    }

    return 1;
}

int bee_db_initialize_tables(sqlite3 *db_handle)
{
    if(!bee_db_perform_query(db_handle, sql_create_table_packagefile(),  NULL))
        return 0;

    if(!bee_db_perform_query(db_handle, sql_create_table_filefunction(), NULL))
        return 0;

    return 1;
}

sqlite3 *bee_db_db_open(int flag)
{
    int ret = 0;
    sqlite3 *db_handle;

    ret = sqlite3_open_v2(bee_db_db_filename(), &db_handle, flag, NULL);

    if(db_handle == NULL) {
        fprintf(stderr, "cannot create database handle, memory allocation failed\n");
        return NULL;
    }

    if (ret != SQLITE_OK) {
        fprintf(stderr, "database handle creation failed: %s\n", sqlite3_errmsg(db_handle));
        ret = sqlite3_close(db_handle);
        if(ret != SQLITE_OK) {
            fprintf(stderr, "closing database handle failed: %s\n", sqlite3_errmsg(db_handle));
        }
        return NULL;
    }

    return db_handle;
}

int bee_db_db_initialize(sqlite3 *db_handle)
{
    /* fill db with tables */
    if(!bee_db_initialize_tables(db_handle))
        return 0;

    return 1;
}

int bee_db_add_data(sqlite3 *db_handle, char *dep_file)
{
    int no_fgets = 0,
        type = 0,
        ret = 0,
        db_commit = 0;
    FILE *file;
    char *query = NULL,
         line[LINE_MAX],
         filename[LINE_MAX],
         key[LINE_MAX],
         value[LINE_MAX];
    const char *tail = NULL;
    sqlite3_stmt *statement = NULL;

    if ((file = fopen(dep_file, "r")) == NULL) {
        fprintf(stderr, "failed to open %s: %s\n", dep_file, strerror(errno));
        return 0;
    }

    sqlite3_exec(db_handle, "BEGIN", 0, 0, 0);

    while(no_fgets || fgets(line, LINE_MAX, file)) {
        no_fgets = 0;
        if(!line[0])
            continue;

        sscanf(line, "[%[^]]]", filename);

        if(filename[0] == '/') {
            type = TYPE_FILE;
        } else {
            type = TYPE_PACKAGE;
        }

        while(fgets(line, LINE_MAX, file)) {
            if(line[0] == '\n')
                continue;

            if(line[0] == '[') {
                no_fgets = 1;
                db_commit = 1;
                break;
            }

            sscanf(line, "%s = %s", key, value);
            if(strcmp(key, "provides") == 0) {
                sql_insert_into_packagefile_add_values(&query, filename, value, "", 0, 0);
                
            }
            
            if(db_commit) {
        ret = sqlite3_prepare_v2(db_handle, query, strlen(query), &statement, &tail);
        if(ret != SQLITE_OK) {
            fprintf(stderr, "failed to prepare query: %s\n", sqlite3_errmsg(db_handle));
            return 0;
        }


                while((ret = sqlite3_step(statement)) != SQLITE_DONE) {
                    if(ret != SQLITE_ROW) {
                        fprintf(stderr, "failed to retrieve row: %s\n", sqlite3_errmsg(db_handle));
                        return 0;
                    }
                }
            }
        }

        ret = sqlite3_finalize(statement);
        if(ret != SQLITE_OK) {
            fprintf(stderr, "failed to destroy query: %s\n", sqlite3_errmsg(db_handle));
            return 0;
        }

    }

    sqlite3_exec(db_handle, "COMMIT", 0, 0, 0);

    fclose(file);

    return 1;
}

int bee_db_add_data_all(sqlite3 *db_handle)
{
    struct dirent *direntry;
    DIR* directory;
    char dep_file[PATH_MAX + 1];

    directory = opendir(bee_metadir());
    if(!directory) {
        fprintf(stderr, "cannot open '%s': %s\n", bee_metadir(), strerror(errno));
    }

    while((direntry = readdir(directory)) != NULL) {
        if(strcmp(direntry->d_name, ".") == 0
         ||strcmp(direntry->d_name, "..") == 0)
            continue;
            	
    	
        sprintf(dep_file, "%s/%s/DEPENDENCIES", bee_metadir(), direntry->d_name);
        
        if(!bee_db_add_data(db_handle, dep_file))
            fprintf(stderr, "failed to add data from %s\n", dep_file);
    }

    closedir(directory);

    return 1;
}

int bee_db_db_rebuild(int argc, char *argv[])
{
    sqlite3 *bee_db_db_handle = NULL;

    if(!bee_db_db_remove())
        return 0;

    if((bee_db_db_handle = bee_db_db_open(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) == NULL) {
        sqlite3_close(bee_db_db_handle);
        return 0;
    }

    if(!bee_db_db_initialize(bee_db_db_handle)) {
        sqlite3_close(bee_db_db_handle);
        return 0;
    }

    if(!bee_db_add_data_all(bee_db_db_handle)) {
        sqlite3_close(bee_db_db_handle);
        return 0;
    }

    sqlite3_close(bee_db_db_handle);

    return 1;
}

int main(int argc, char *argv[])
{
    int action;

    check_env();

    if (argc < 2) {
        fprintf(stderr, "not enough arguments\n");
        return 1;
    }

    action = parse_action(argv[1]);

    argv++;
    argc--;

    switch (action) {
        case BEE_DB_ACTION_REBUILD:
            if(!bee_db_db_rebuild(argc, argv))
                return 1;
            break;
/*
        case UPDATE:
            return bee_dep_update(argc, argv);

        case REMOVE:
            return bee_dep_remove(argc, argv);

        case LIST:
            return bee_dep_list(argc, argv);

        case CONFLICTS:
            return bee_dep_conflicts(argc, argv);
*/
        default:
            printf("unknown option specified\n");
            return 1;
    }

    return 0;
}
