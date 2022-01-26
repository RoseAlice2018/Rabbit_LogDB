#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <string>


#include "ldb/db.h"
#include "ldb/slice.h"
#include "ldb/status.h"
#include "port/thread_annotations.h"
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
        Status Write(const WriteOptions& options,WriteBatch* updates)override;
        Status Get(const ReadOptions& options,const Slice& key,
                        std::string* value)override;
        Iterator* NewIterator(const ReadOptions&) override;
        const Snapshot* GetSnapshot() override;
        void ReleaseSnapshot(const Snapshot* snapshot) override;
        bool GetProperty(const Slice& property, std::string* value) override;
        void GetApproximateSizes(const Range* range, int n, uint64_t* sizes) override;
        void CompactRange(const Slice* begin, const Slice* end) override;

        private:
            friend class DB;
            struct CompactionState;
            struct Writer;

            // Information for a manual compaction
            struct ManualCompaction{
                int level;
                bool done;
                const InternalKey* begin;   // null means beginning of key range 
                const InternalKey* end;     // null means end of key range
                InternalKey tmp_storage;    // Used to keep track of compaction progress
            };

              // Per level compaction stats.  stats_[level] stores the stats for
              // compactions that produced data for the specified "level".
            struct CompactionStats {
                CompactionStats() : micros(0), bytes_read(0), bytes_written(0) {}

                void Add(const CompactionStats& c) {
                this->micros += c.micros;
                this->bytes_read += c.bytes_read;
                this->bytes_written += c.bytes_written;}

                int64_t micros;
                int64_t bytes_read;
                int64_t bytes_written;
            };

            Iterator* NewInternalIterator(const ReadOption&,
                                SequenceNumber* latest_snapshot,
                                uint32_t* seed);
            
            Status NewDB();

            // Recover the descriptor from persistent storage. May do a significant
            // amount of work to recover recently logged updates. Any changes to be made to
            // the descriptor are added to *edit.
            Status Recover(VersionEdit* edit, bool* save_manifest)
                EXCLUSIVE_LOCKS_REQUIRED(mutex_);

            void MaybeIgnoreError(Status* s) const;

            // Delete any unneeded files and stale in-memory entries.
            void RemoveObsoleteFiles() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

            // Compact the in-memory write buffer to disk. Switches to a new 
            // log-file/memtable and writes a new descriptor iff successful.
            // Errors are recorded in bg_error_.
            void CompactMemTable() EXCLUSIVE_LOCKS_REQUIRED(mutex_);


            Status RecoverLogFile(uint64_t log_number,bool last_log,bool* save_manifest,
                                VersionEdit* edit,SequenceNumber* max_sequence)
                                EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            Status WriteLevel0Table(MemTable* mem,VersionEdit* edit,Version* base)
                    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            
            Status MakeRoomForWrite(bool force/* compact even if there is room */)
                    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            
            WriteBatch* BuildBatchGroup(Writer** last_writer)
                    EXCLUSIVE_LOCKS_REQUIRED(mutex_);
            
            void RecordBackgroundError(const Status& s);

            


    };   
    
}


#endif