#include <assert.h>
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

int sql_insert_into_packagefile_add_values(char **query, char *packagename, char *filename, char *md5sum, char *size, char *flags)
{
    int length = 0, addlen = 0;
    char *start = "INSERT INTO PackageFile VALUES ";
    char *add = NULL;

    assert(query);
    assert(package);
    assert(filename);
    assert(md5sum);
    assert(size);
    assert(flags);

    addlen += 2 + 4;
    addlen += strlen(packagename);
    addlen += strlen(filename);
    addlen += strlen(md5sum);
    addlen += strlen(size);
    addlen += strlen(flags);
    
    add = calloc(addlen + 1, sizeof(char));
    if(add == NULL) {
        fprintf(stderr, "failed to create sql string 'INSERT INTO PackageFile ..': %s\n", strerror(errno));
        return 0;
    }
    
    if((sprintf(add, "(%s,%s,%s,%s,%s)", packagename, filename, md5sum, size, flags) != addlen) {
        fprintf(stderr, "sprintf failed: %s\n", strerror(errno));
        return 0;	
    }    
    
    if(*query) {
    	length += strlen(*query);
    	length += 1; /* add a comma */
    } else {
        length += strlen(start);
    }	

    *query = realloc(*query, (length + 1) * sizeof(char));
    if(! *query) {
        fprintf(stderr, "memory allocation failed: %s\n", strerror(errno));
        return 0;
    }
    
    
    
    return 1;
}
