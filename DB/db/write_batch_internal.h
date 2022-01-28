#ifndef STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_
#define STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_

#include "ldb/write_batch.h"
#include "db/dbformat.h"
namespace Rabbitdb{
    class MemTable;

    // WriteBatchInternal provides static methods for manipulating a 
    // WriteBatch that we don't want in the public WriteBatch interface. 
    class WriteBatchInternal{
        public:
            // Return the number of entries in the batch
            static int Count(const WriteBatch* batch);

            // Set the count for the number of entries in the batch.
            static void Setcount(WriteBatch* batch,int n);

            // Return the sequence number for the start of this batch
            static SequenceNumber Sequence(const WriteBatch* batch);

            // Store the specified number as the sequence number for the start
            // of this batch.
            static void SetSequence(WriteBatch* batch,SequenceNumber seq);

            static Slice Contents(const WriteBatch* batch){return Slice(batch->rep_);}

            static size_t ByteSize(const WriteBatch* batch){return batch->rep_.size();}

            static void SetContents(WriteBatch* batch,const Slice& contents);

            static Status InsertInto(const WriteBatch* batch,MemTable* memTable);

            static void Append(WriteBatch* dst,const WriteBatch* src);
    };
}



#endif