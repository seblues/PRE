#ifndef DATABASE_H
#define DATABASE_H

#include "utils.h"
/*
 * lib to use for the modification of the zigbee table, this table 
 * contains static addresses and dynamic adresses of all the modules
 * in the network 
 * 
 * table name: addr 
 * _________________________
 * |sta				|dyn	|
 * |________________|_______|
 * |0013A2004089EBE1|DCFB	|
 * |...				|...	|
 * 
 */

//insert a new module in the addr table
int insertModule(sqlite3* db, char* macAddr, char* netAddr);

//remove the module from the addr table 
int deleteModule(sqlite3* db, char* macAddr);

//init the database and create the addr table if not exists
int initDb(sqlite3* db);

//size of netAddrDest must be 4
int getNetAddr(sqlite3* db, char* macAddr, char* netAddrDest);

int getAllMacAddr(sqlite3* db, struct List* macAddrList);

int insertValue(sqlite3* db, unsigned int type, char* macAddr, unsigned int id, float value, unsigned int timestamp);

int callbackDb(void *pList, int argc, char **argv, char **azColName);


#endif
