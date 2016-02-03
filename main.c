#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "sqlite3.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <string.h>

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <stdlib.h>

#include <nm-client.h>
#include <nm-device.h>
#include <nm-device-wifi.h>
#include <nm-access-point.h>
#include <NetworkManager.h>
#include <nm-utils.h>

static int epochCount = 0;
static sqlite3* wifiDb = NULL;
static FILE* logfile = NULL;


bool insert_access_point(const char* name)
{
    char *err_msg = NULL;

    char *sql =
        sqlite3_mprintf("INSERT INTO AccessPoints (Name) VALUES ('%q')", name);

    if (sqlite3_exec(wifiDb, sql, NULL, NULL, &err_msg) != SQLITE_OK)
    {
        fprintf(logfile, "Error inserting into AccessPoints table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_free(sql);
        return false;
    }

    sqlite3_free(sql);
 
    return true;
}

static int selectAPCallback(void *data, int i, char **a, char **b)
{
    // if this gets called, we found a match for this ap
    bool* found = (bool*)data;
    *found = true;

    return 0;
}

bool isWifiKnown(const char * name)
{
    char* sql = sqlite3_mprintf("SELECT * FROM AccessPoints WHERE Name = '%q'",
        name);

    char *zErrMsg = NULL;
    int rc;
    bool found = false;

    rc = sqlite3_exec(wifiDb, sql, selectAPCallback, (void*)&found, &zErrMsg);
    sqlite3_free(sql); 

    if (rc != SQLITE_OK)
    {
        fprintf(logfile, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
 
    return found;
}

static void upsert_access_point(NMAccessPoint *ap)
{
    // build ap name


    const GByteArray *ssid; 
    const char *hwaddr; 
    char* ssid_str = NULL;
    GString *entry_str;


    ssid = nm_access_point_get_ssid(ap);
    hwaddr = nm_access_point_get_bssid(ap);

    entry_str = g_string_new(NULL);
    
    if (ssid != NULL)
    {
        ssid_str = nm_utils_ssid_to_utf8(ssid);
        g_string_append(entry_str, ssid_str);
    }

    g_string_append(entry_str, "-");
    g_string_append(entry_str, hwaddr);

    if (isWifiKnown(entry_str->str))
    {
        //printf("%s is known\n", entry_str->str);
        // TODO: update stats for this?
    }
    else
    {
        fprintf(logfile, "%10d: Adding: %s\n", epochCount, entry_str->str);
        //printf("Adding: %s\n", entry_str->str);
        // TODO: insert this to the db
        insert_access_point(entry_str->str);
    }

    g_free(ssid_str);
    g_string_free(entry_str, TRUE);
}


static void show_wifi_device_info(NMDevice *device)
{
    NMAccessPoint *active_ap = NULL;
    const GPtrArray *aps;
    const GByteArray *active_ssid;
    char *active_ssid_str = NULL;
    int i;

    // Get active AP
    if (nm_device_get_state(device) == NM_DEVICE_STATE_ACTIVATED)
    {
        if ((active_ap = 
             nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(device))))
        {
            active_ssid = nm_access_point_get_ssid(active_ap);
            active_ssid_str = nm_utils_ssid_to_utf8(active_ssid);
        }
    }

    g_free(active_ssid_str);

    // get all APs of the Wi-Fi device
    aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(device));

    for (i = 0; aps && (i < aps->len); i++)
    {
        NMAccessPoint *ap = g_ptr_array_index(aps, i);
        upsert_access_point(ap);
    }
}

void scanForWifi()
{
    DBusGConnection *bus;
    NMClient *client;
    const GPtrArray *devices;
 
    bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
 
    client = nm_client_new();
 
    if (!client) 
    { 
        dbus_g_connection_unref(bus);
        fprintf(logfile, "Error: Could not create NMClient.\n");
    } 
 
    devices = nm_client_get_devices(client);

    int i = 0; 
    for (; devices && (i < devices->len); i++)
    { 
        NMDevice *device = g_ptr_array_index (devices, i);
        if (NM_IS_DEVICE_WIFI(device))
        { 
            show_wifi_device_info(device);
        } 
    }
}


sqlite3* init_wifi_db()
{
    sqlite3 *db = NULL;

    int rc = sqlite3_open("wifi.db", &db);

    if (rc != SQLITE_OK)
    {
        fprintf(logfile, "Cannot open wifi database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
    }
    else
    {
        char *err_msg = NULL;
        char *sql = "CREATE TABLE IF NOT EXISTS AccessPoints(id INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT NOT NULL);";

        rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);

        if (rc != SQLITE_OK)
        {
            fprintf(logfile, "Error creating AccessPoints table: %s\n", err_msg);
            sqlite3_free(err_msg);
            sqlite3_close(db);
        }
    }

    return db;
}


void sleepFor(int seconds)
{
    sleep(seconds);
    epochCount += seconds;
}

void performChildScan()
{
    // since nothing I try causes NetworkManager to
    // refresh the list of access points I get other
    // than restarting the process, I'll just do the
    // request in a new process each time

    pid_t pid;
    int status;
 
    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // child process
        logfile = fopen("activitylog.txt", "a+");
 
        // open db
        wifiDb = init_wifi_db();
        if (wifiDb == NULL)
        {
            exit(EXIT_FAILURE);
        }

        scanForWifi();
 
        // close db
        sqlite3_close(wifiDb);

        fclose(logfile);
 
        exit(EXIT_SUCCESS);
    }
    else
    {
        // wait for child to exit
        wait(&status);
    }
}

int main()
{
    pid_t pid, sid;

    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        // exit the parent process
        exit(EXIT_SUCCESS);
    }

    umask(0);    

    sid = setsid();
    if (sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0)
    {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while (1)
    {
        performChildScan();
        sleepFor(5);
    }

    return 0;
}
