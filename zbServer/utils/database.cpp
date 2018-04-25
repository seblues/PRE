#include <string.h>
#include <sqlite3.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "database.h"

int initDb(sqlite3* db){ 
    std::string sql =	"CREATE TABLE IF NOT EXISTS addr("\
                        "sta CHAR(16) PRIMARY KEY NOT NULL, "\
                        "dyn CHAR(4) UNIQUE NOT NULL);";
    std::cout << sql << std::endl;

    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, 0);

    sql =   "CREATE TABLE IF NOT EXISTS val("\
            "sta CHAR(16) PRIMARY KEY NOT NULL, "\
            "type INT(1), "\
            "id INT(2), "
            "value FLOAT(6,4), "
            "timestamp INT(10)"
            ");";
    std::cout << sql << std::endl;

    rc = sqlite3_exec(db, sql.c_str(), 0, 0, 0);

	return rc;
}

//______ADDR_API____________________________________

int insertModule(sqlite3* db, char* macAddr, char* netAddr){

    //convert from hex to caract
    unsigned char macAddrStr[17];
    unsigned char netAddrStr[5];
    buffToStr(macAddrStr,(unsigned char*)macAddr,8);
    buffToStr(netAddrStr,(unsigned char*)netAddr,2);

    //create request
    std::string sqlRequest =    "INSERT INTO addr(sta,dyn) VALUES ('" +
                                std::string((char*)macAddrStr) + "','" +
                                std::string((char*)netAddrStr) + "');";

    std::cout << sqlRequest << std::endl;

    //execute request
    int rc = sqlite3_exec(db, sqlRequest.c_str(), 0, 0, 0);

    return rc;
}


int deleteModule(sqlite3* db, char* macAddr){
	unsigned char macAddrStr[17];
	buffToStr(macAddrStr,(unsigned char*)macAddr,8);

    std::string sqlRequest = "DELETE FROM addr WHERE sta='" + std::string((char*)macAddrStr) + "';";
    std::cout << sqlRequest << std::endl;

    int rc = sqlite3_exec(db, sqlRequest.c_str(), 0, 0, 0);
	return rc;
}

//size of netAddr must be 4
int getNetAddr(sqlite3* db, char* macAddr, char* netAddrDest){
    List netAddrList = {0};
    unsigned char macAddrStr[17];

    buffToStr(macAddrStr, (unsigned char*)macAddr, 8);

    std::string sqlRequest = "SELECT dyn FROM addr WHERE sta='" + std::string((char*)macAddrStr) + "';";
    std::cout << sqlRequest << std::endl;

    int rc = sqlite3_exec(db, sqlRequest.c_str(), callbackDb, (void*)&netAddrList, 0);

    strToBuff(netAddrDest, netAddrList.values[0], 4);

    return rc;
}

int getAllMacAddr(sqlite3* db, struct List* macAddrList){

    static struct List strList;
    strList.index = 0;
    macAddrList->index = 0;

    std::string sqlRequest = "SELECT sta FROM addr;";
    std::cout << sqlRequest << std::endl;

    int nbOfAddr = sqlite3_exec(db, sqlRequest.c_str(), callbackDb, (void*)&strList, 0);

    unsigned int i = 0;
    for (i=0; i<strList.index; i++){
        strToBuff(macAddrList->values[i], strList.values[i], 16);
        macAddrList->index++;
    }

    return nbOfAddr;
}

//get all mac address contained by the table
int callbackDb(void *pValueList, int argc, char **argv, char **azColName){

    struct List* valueList = (struct List*)pValueList;
    unsigned int i = 0;
    for(i = 0; i<argc; i++){
        strcpy((valueList)->values[valueList->index], argv[i]);
    }
    ((struct List *)valueList)->index++;

    return 0;
}


int insertValue(sqlite3* db, unsigned int type, char* macAddr, unsigned int id, float value, unsigned int timestamp){

    //convert from hex to caract
    unsigned char macAddrStr[17];
    buffToStr(macAddrStr,(unsigned char*)macAddr,8);

    std::string sqlRequest =    "INSERT INTO val VALUES(" +
                                std::to_string(type) + ", '" +
                                std::string((char*)macAddrStr) + "', " +
                                std::to_string(id) + ", " +
                                std::to_string(value) + ", " +
                                std::to_string(timestamp) + ");";
    std::cout << sqlRequest << std::endl;

    int rc = sqlite3_exec(db, sqlRequest.c_str(), 0, 0, 0);

    return 0;
}
