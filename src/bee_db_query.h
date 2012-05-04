extern char *sql_transaction_new(void);
extern int sql_transaction_add(char **transaction, char *query);
extern int sql_transaction_end(char **transaction);

extern char *sql_create_table_packagefile(void);
extern char *sql_create_table_filefunction(void);

extern char *sql_insert_into_packagefile(void);
extern char *sql_insert_into_filefunction(void);

extern int sql_insert_into_packagefile_add_values(char **query, char *package, char *filename, char *md5sum, int size, int flags);