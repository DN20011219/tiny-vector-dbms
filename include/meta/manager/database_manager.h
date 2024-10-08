// Copyright (c) 2024 by dingning
//
// file  : TODO
// since : 2024-07-TODO
// desc  : TODO.

#ifndef TODO
#define TODO

#include <map>
#include <string>
#include <shared_mutex>
#include <mutex>

#include "../db/db.h"
#include "../table/column_table.h"
#include "../../storage/block_file_management.h"
#include "../../storage/file_management.h"
#include "../../utils/cal_file_url_util.h"
#include "../../config.h"


using std::map;
using std::string;
using std::shared_mutex;
using std::shared_lock;
using std::lock_guard;

namespace tiny_v_dbms {

class DatabaseControllSlot
{
    friend class DatabaseManager;

    DB db;
    // shared_mutex read_or_write_mutex;

public:

    DatabaseControllSlot& operator=(DatabaseControllSlot&& other) {
        db = std::move(other.db);
        return *this;
    }

    DatabaseControllSlot(DB new_db)
    {
        db = std::move(new_db);
    };
};

class DatabaseManager
{

private:
    map<string, DatabaseControllSlot> opened_db;
    // map<string, shared_mutex*> opened_db_mutex;

    // vector<lock_guard<shared_mutex>*> locks;

    BlockFileManagement* bfmm;
    FileManagement* fm;
public:

    // When create a DatabaseManager, it will open the default db automically
    DatabaseManager()
    {
        bfmm = new BlockFileManagement();
        fm = new FileManagement();

        DB default_db;
        if (!OpenDefaultDb(default_db))
        {
            throw std::runtime_error("Can not open default databse, please check database has been installed!");
        }

        DatabaseControllSlot default_slot(default_db);
        opened_db[DEFAULT_DB_FILE_NAME] = std::move(default_slot);
    }

    ~DatabaseManager()
    {
        delete bfmm;
        delete fm;
    }

    // When DatabaseManager is created, it will open the default db automatically, this db has all db's header information.
    bool OpenDefaultDb(DB& db)
    {
        char* data;
        db.DeserializeDBFile(data, fm->ReadFile(CalFileUrlUtil::GetDefaultDbFile(DEFAULT_DB_FILE_NAME), data));

        // read the first block of deafult table
        TableBlock table_block;
        bfmm->ReadOneTableBlock(db.default_table_header_file_path, 0, table_block);

        default_amount_type tables_num = table_block.table_amount;
        while(tables_num > 0)
        {
            ColumnTable new_table;
            new_table.Deserialize(table_block.data, table_block.tables_begin_address[tables_num]);
            db.tables.emplace_back(new_table);
            tables_num--;
        }
        
        // read next blocks if exist
        if (table_block.next_block_pointer != 0x0)
        {
            LoadTables(db, db.default_table_header_file_path, table_block.next_block_pointer);
        }
        
        return true;
    }

    // load tables in input db, and serialize to DB& db
    void LoadTables(DB& db, string table_header_file_url, default_address_type offset)
    {
        TableBlock* table_block = new TableBlock(); // try get one block in memory
        bfmm->ReadOneTableBlock(table_header_file_url, 0, *table_block); // read file to memory

        // deserialize
        default_amount_type tables_num = table_block->table_amount;
        while(tables_num > 0)
        {
            ColumnTable new_table;
            new_table.Deserialize(table_block->data, table_block->tables_begin_address[tables_num]);
            db.tables.emplace_back(new_table);
            tables_num--;
        }

        // if has next block, contine deserialize
        if (table_block->next_block_pointer != 0x0)
        {
            default_address_type next_block_offset = table_block->next_block_pointer;
            delete table_block;  // release block
            LoadTables(db, table_header_file_url, next_block_offset);
        }
    }

    // Try open one db, if opened, set db as a copy of DB and return true, else set null_ptr and return false
    // This function can support many thread at one time, and don't need close();
    // The change of db_file, table_header will be not be saved or read by other thread;
    bool OpenDbForRead(string db_name, DB& db)
    {
        if (db_name == DEFAULT_DB_FILE_NAME)
        {
            throw std::runtime_error("Can not open base db for user");
        }

        // try find db in map
        if (opened_db.find(db_name) != opened_db.end())
        {
            // lock the mutex at read mode
            // lock_guard<shared_mutex>(*opened_db_mutex[db_name]);
            
            db = opened_db[db_name].db;
            return true;
        }

        // open db file and table header
        DB new_db;
        string table_header_file_url = CalFileUrlUtil::GetDefaultDbFile(db_name);
        char* data;
        new_db.DeserializeDBFile(data, fm->ReadFile(CalFileUrlUtil::GetDefaultDbFile(DEFAULT_DB_FILE_NAME), data));

        // read the first block of deafult table
        TableBlock table_block;
        bfmm->ReadOneTableBlock(db.default_table_header_file_path, 0, table_block);

        default_amount_type tables_num = table_block.table_amount;
        while(tables_num > 0)
        {
            ColumnTable new_table;
            new_table.Deserialize(table_block.data, table_block.tables_begin_address[tables_num]);
            db.tables.emplace_back(new_table);
            tables_num--;
        }
        
        // read next blocks if exist
        if (table_block.next_block_pointer != 0x0)
        {
            LoadTables(db, table_header_file_url, table_block.next_block_pointer);
        }

        // copy db
        db = new_db;
        
        // add to map
        opened_db[db_name] = DatabaseControllSlot(new_db);
        // opened_db_mutex[db_name] = new shared_mutex();
        return true;
    }

    // Try open one db, if opened, set db_pointer as the address of DB and return true, else set null_ptr and return false
    // This function must be used with CloseDb() ! If not, all changes about db, table struct will lost.
    bool OpenDb(string db_name, DB*& db_pointer)
    {

    }

    // Open one db for change, it will kill all other thread which is using this db.
    // Used when try update db or tables immediately.
    bool DepriveOpenDb(string db_name)
    {

    }

    // Close one DB, and save all changes to disk.
    // If the input db has been closed or not opened, will throw a error
    bool CloseDb(string db_name)
    {

    }


};

}

#endif // TODO