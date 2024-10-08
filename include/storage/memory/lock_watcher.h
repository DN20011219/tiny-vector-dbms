// Copyright (c) 2024 by dingning
//
// file  : lock_watcher.h
// since : 2024-08-15
// desc  : TODO.

#ifndef VDBMS_STORAGE_MEMORY_LOCK_WATCHER_H_
#define VDBMS_STORAGE_MEMORY_LOCK_WATCHER_H_

#include <string>
#include <vector>
#include <map>
#include <mutex>

#include "./buffer_pool.h"
#include "./block_slot.h"
#include "../block_file_management.h"
#include "../../config.h"
#include "../../utils/cal_file_url_util.h"

namespace tiny_v_dbms {

class LockWatcher
{

private:
    BufferPool* buffer_pool;
    BlockFileManagement* bfmm;

    SlotTool* slot_tool;

    std::vector<BlockSlot*> slots;
    std::map<SlotSign, BlockSlot*> slots_map;
    std::mutex slots_map_mutex;
    
public:
    CalFileUrlUtil* cal_url_util;

    LockWatcher(default_amount_type slots_amount);

    ~LockWatcher();

    void LoadBlockForRead(std::string db_name, std::string table_name, default_address_type offset, DataBlock& block);

    void LoadBlockForWrite(std::string db_name, std::string table_name, default_address_type offset, DataBlock& block);

    void LoadBlockForRead(std::string db_name, std::string table_name, default_address_type offset, TableBlock& block);

    void LoadBlockForWrite(std::string db_name, std::string table_name, default_address_type offset, TableBlock& block);

    bool UpgradeLock(std::string db_name, std::string table_name, default_address_type offset);

    void ReleaseReadingBlock(std::string db_name, std::string table_name, default_address_type offset, DataBlock& block);
    void ReleaseReadingBlock(std::string db_name, std::string table_name, default_address_type offset, TableBlock& block);

    void ReleaseWritingBlock(std::string db_name, std::string table_name, default_address_type offset, DataBlock& block);
    void ReleaseWritingBlock(std::string db_name, std::string table_name, default_address_type offset, TableBlock& block);

    default_address_type CreateNewBlock(std::string db_name, std::string table_name, DataBlock& block);
    default_address_type CreateNewBlock(std::string db_name, std::string table_name, TableBlock& block);
};

}

#endif // VDBMS_STORAGE_MEMORY_LOCK_WATCHER_H_