#ifndef STORAGE_LEVELDB_DB_DBFORMAT_H_
#define STORAGE_LEVELDB_DB_DBFORMAT_H_

#include <stddef.h>
#include <stdint.h>
namespace Rabbitdb{
    typedef uint64_t SequenceNumber; 


    // Modules in this directory should keep internal keys wrapped inside
    // the following class instead of plain strings so that we do not
    // incorrectly use string comparsions instead of an InternalKeyComparator
    class InternalKey{

    };
}


#endif