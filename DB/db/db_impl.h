#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <string>


#include "ldb/db.h"
#include "ldb/slice.h"
namespace Rabbitdb{
    class MemTable;

    class DBImpl:public DB{
        public:
        DBImpl(const Options& options,const std::string& dbname);

        DBImpl(const DBImpl&) = delete;
        DBImpl& operator=(const DBImpl&) = delete;

        ~DBImpl() override;

        //Implementations of the DB interface
        Status Put(const WriteOptions&,const Slice& key,
                    const Slice& value)override;
        Status Delete(const WriteOptions&,const Slice& key)override;

    };   
    
}


#endif