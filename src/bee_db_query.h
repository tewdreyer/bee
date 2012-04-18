extern char *sql_transaction_new(void);
extern int sql_transaction_add(char **transaction, char *query);
extern int sql_transaction_end(char **transaction);

extern char *sql_create_table_packagefile(void);
extern char *sql_create_table_filefunction(void);

extern char *sql_insert_into_packagefile(void);
extern char *sql_insert_into_filefunction(void);
