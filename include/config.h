// Copyright (c) 2024 by dingning
//
// file  : config.h
// since : 2024-07-16
// desc  : Basic config of this project, contains the default vector type ...

#ifndef VDBMS_CONFIG_H_
#define VDBMS_CONFIG_H_

#include "meta/vector.h"

namespace tiny_v_dbms {
    // config about this dbms

    /*
    || means it is a folder, string means it is a file
    folder struct descripition: Each db has one folder named as its name, adn all data is stored in this folder.
    Each db has one default table, this table will store all tables name in this db. But in the default db, this table will store all the dbs name.

    |root|
    --|base_db|
    ----base_db.tvdb
    ----|tables|
    ------default_table.tvdbb (store the table header)
    ------|data|
    --------default_table.data (contains all data of default_table table)
    */
    #define INSTALL_PATH_CACHE_FILE "./install_url.txt"         // the uri of config file, use to store the path where installed this dbms

    #define DEFAULT_DB_FOLDER_NAME "base_db"                 // the name of installed default db
    #define DEFAULT_DB_FILE_NAME "base_db"                  // the name of default db file
    #define DB_FILE_SUFFIX ".tvdb"                             // the suffix of all db file

    #define DEFAULT_TABLE_FOLDER "tables"                       // this folder will store all data about tables , it is under "db_name"(DB_FOLDER_NAME) folder
    #define DEFAULT_TABLE_NAME "default_table"                 // the name of default db table file, each db has one default_table to store all tables header information in db
    #define TABLE_FILE_SUFFIX ".tvdbb"                         // the suffix of table header file
   
    #define DEFAULT_TABLE_COLUMN_NAME_ONE "db_names"            // the default column name of default table, this table only has one column, to store all db name sin this system
    #define DEFAULT_TABLE_COLUMN_NAME_TWO "tables_names"         // the default column name of default table, used in user's db.

    #define DEFAULT_TABLE_DATA_FOLDER "data"                  // this folder will store all exact data of this db, it is under "tables"(DEFAULT_TABLE_FOLDER) folder
    #define DEFAULT_TABLE_DATA_FILE_NAME "default_table"        // the name of default db table data file
    #define TABLE_DATA_FILE_SUFFIX ".data"                      // the suffix of table data file

    #define DEFAULT_TABLE_LOG_FOLDER "log"
    #define DEFAULT_TABLE_LOG_FILE_NAME "default_table"
    #define TABLE_LOG_FILE_SUFFIX ".log"  

    // config about storage
    #define MEMORY_SIZE 10737418239 / 4            // the size of the memory, any memory using need to acquire space here, 10737418239 byte (1g) / 4 = 256 mb
    #define BLOCK_SIZE 4096                    // the size of one block is 4096 byte (4kb)
    #define LOG_MANAGER_INSRANCE_AMOUNT 4096    // the log manager amount, it should as same as block amout in memory_management


    // config about meta data toe
    #define BASE_VECTOR Vector<int, double>     // basic vector, LENGTH_TYPE is int, DATA_TYPE is double

    // config about default data type
    #define default_length_size int            // use int as the default type of length.
    #define default_amount_type int             // use int as the default type of item amount.
    #define default_enum_type int               // use int to means enum item.
    #define default_address_type int            // use int as the default type to store address.
    #define default_pointer_size 4              // use 32bit as the default size of pointer, this may be need to change for adaption to 64bit device

    // config about enum data
    enum TABLE_TYPE {DEFAULT, COMMON};          // type of table, now no use
    enum column_type {NUMBER, VECTOR, VCHAR};
    enum column_length {NUMBER_LENGTH = 4, VCHAR_LENGTH = 50};
    enum column_index_type {NONE, FLAT};

    // config about client and server
    #define CONNECTOR_MESSAGE_KEY ftok("tvdbms_connect", 7) 
    #define CONNECTOR_MESSAGE_ID 40286          // this id is used to confirm the msg queue used by connecter and clients
    #define WORKER_MESSAGE_ID 40287             // this id is used to confirm the msg queue used by workers and clients
    #define CONNECTOR_MSG_TYPE_SEND    100      // this type is used when server send msg to client
    #define CONNECTOR_MSG_TYPE_RECV    200      // this type is used when client send msg to server

    #define MSG_DATA_LENGTH 128                 // config about msg used by connecter
    #define IP_LENGTH 15
    #define PORT_LENGTH 4
    #define IDENTITY_LENGTH 4
    #define CONNECT_DB_NAME_LENGTH_LENGTH 4
    #define CONNECT_DB_NAME_LENGTH  100

    #define SQL_MSG_DATA_LENGTH 200             // config about msg used by worker, here limit the max length is 200
    #define SQL_LENGTH_LENGTH 4

}

#endif // VDBMS_CONFIG_H_