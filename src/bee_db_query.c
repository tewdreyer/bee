#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *sql_transaction_new(void)
{
    char *start = "";
    char *q = NULL;

    q = strdup(start);
    if(q == NULL) {
        fprintf(stderr, "memory allocation failed: %s", strerror(errno));
        return NULL;
    }

    return q;
}

int sql_transaction_add(char **transaction, char *query)
{
    int length = 0;

    length  = strlen(*transaction);
    length += strlen(query);

    *transaction = realloc(*transaction, length + 1);
    if(*transaction == NULL) {
        fprintf(stderr, "failed to allocate memory for transaction: %s", strerror(errno));
        return 0;
    }

    strcat(*transaction, query);

    return length;
}

int sql_transaction_end(char **transaction)
{
    char *end = "";//"COMMIT;";
    int length = 0;

    length  = strlen(*transaction);
    length += strlen(end);

    *transaction = realloc(*transaction, length + 1);
    if(*transaction == NULL) {
        fprintf(stderr, "failed to allocate memory for transaction: %s", strerror(errno));
        return 0;
    }

    strcat(*transaction, end);

    return length;
}

char *sql_create_table_packagefile(void)
{
    char *q = "CREATE TABLE PackageFile ( "
                  "Package TEXT, "
                  "File TEXT, "
                  "MD5 TEXT, "
                  "Size INTEGER, "
                  "Flags INTEGER, "
                  "PRIMARY KEY ( Package, File ) );";

    return q;
}

char *sql_create_table_filefunction(void)
{
    char *q = "CREATE TABLE FileFunction ( "
                  "File TEXT, "
                  "Function TEXT, "
                  "Flags INTEGER, "
                  "PRIMARY KEY ( File, Function ) );";

    return q;
}

char *sql_insert_into_packagefile(void)
{
    return "INSERT INTO PackageFile VALUES (?, ?, ?, ?, ?);";
}

char *sql_insert_into_filefunction(void)
{
    return "INSERT INTO FileFunction VALUES (?, ?, ?);";
}
