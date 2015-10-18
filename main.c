#include <stdio.h>
#include <stdbool.h>
#include "sqlite3.h"

sqlite3* init_wifi_db()
{
    sqlite3 *db = NULL;

    int rc = sqlite3_open("wifi.db", &db);

    if (rc != SQLITE_OK)
    {
        printf("Cannot open wifi database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
    }
    else
    {
        char *err_msg = NULL;
        char *sql = "CREATE TABLE IF NOT EXISTS AccessPoints(id INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT NOT NULL);";

        rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);

        if (rc != SQLITE_OK)
        {
            printf("Error creating AccessPoints table: %s\n", err_msg);
            sqlite3_free(err_msg);
            sqlite3_close(db);
        }
    }

    return db;
}

bool insert_access_point(sqlite3* db)
{
    char *err_msg = NULL;
    char *sql = "INSERT INTO AccessPoints (Name) VALUES ('NewAccessPoint');";

    if (sqlite3_exec(db, sql, NULL, NULL, &err_msg) != SQLITE_OK)
    {
        printf("Error inserting into AccessPoints table: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

int main()
{
    printf("Starting main...\n");

    sqlite3* wifiDb = init_wifi_db();

    if (wifiDb == NULL)
    {
        return 1;
    }

    if (insert_access_point(wifiDb) == false)
    {
        return 1;
    }

    sqlite3_close(wifiDb);

    int last_id = sqlite3_last_insert_rowid(wifiDb);
    printf("Last inserted row id was %d\n", last_id);

    printf("Done.\n");
    return 0;
}
