#include <sqlite3.h>
#include <stdio.h>
#include "database.h"

int main(){
    //init dataBase
    sqlite3 *db;
    unsigned char macAddr0[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 , 0x07};
    unsigned char netAddr0[2] = {0x00, 0x01};

    unsigned char macAddr1[8] = {0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 , 0x07};
    unsigned char netAddr1[2] = {0x01,0x00};

    int rc = sqlite3_open("prgm.db", &db);
    initDb(db);

    insertModule(db, macAddr0, netAddr0);
    insertModule(db, macAddr1, netAddr1);

    insertValue(db, 0, macAddr0, 1, 10.2, 14915020);

    struct List macAddrList;
    int nbOfAddr = getAllMacAddr(db, &macAddrList);


    return 0;
}
