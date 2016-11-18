#include <stdio.h>
#include <string.h>

#include "ctapi_mysql.h"


#define MAX_LINE    1024

#define user "sales03@tti-mould.sinanet.com"
#define subject "This is test subject"
#define db_host "mysql_host"
#define db_port "mysql_port"
#define db_user "mysql_user"
#define db_pass "mysql_pass"
#define db_name "mysql_db"
#define db_timeout 1

int main(int argc, char const *argv[])
{
	static MYSQL_CONN *db = NULL;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char sql[MAX_LINE] = {0};
    int n = 0;

    db = ct_mysql_connect(db_host, atoi(db_port), db_user, db_pass, db_name, db_timeout);
    if (db == NULL) {
        printf("mysql connect fail:%s\n", db_host);
        return -1;
    }

    // escape special string
    char escape_subject[MAX_LINE] = {0};
    n = mysql_real_escape_string(db->msp, escape_subject, subject, strlen(subject));
    escape_subject[n] = '\0';
    char escape_user[1024] = {0};
    n = mysql_real_escape_string(db->msp, escape_user, user, strlen(user));
    escape_user[n] = '\0';

    // insert or update
    n = snprintf(sql, sizeof(sql), "update mail_info set subject = '%s' where default_email = '%s' limit 1", escape_subject, escape_user);
    if (ct_mysql_query(db, sql, strlen(sql), 1) != 0) {
        printf("ct_mysql_query error, sql:%s\n", sql);
        ct_mysql_disconnect(db);
        return -1;
    }
    if (mysql_affected_rows(db->msp) != 1) {
        printf("update error, sql:%s\n", sql);
        ct_mysql_disconnect(db);
        return -1;
    }

    // select
    n = snprintf(sql, sizeof(sql), "select * from mail_info where default_email = '%s'", escape_user);
    if (ct_mysql_query(db, sql, strlen(sql), 1) != 0) {
        printf("ct_mysql_query error\n");
        ct_mysql_disconnect(db);
        db = NULL;
        return -1;
    }

    res = ct_mysql_store_result(db, 1);
    if (res == NULL) {
        printf("ct_mysql_store_result error\n");
        ct_mysql_disconnect(db);
        db = NULL;
        return -1;
    }

    while ((row = mysql_fetch_row(res)) != NULL) {
        printf("%s %s %s %s %s %s \n",
            row[0], row[1], row[2], row[3], row[4], row[5]);
    }

    mysql_free_result(res);
    ct_mysql_disconnect(db);

	return 0;

}

