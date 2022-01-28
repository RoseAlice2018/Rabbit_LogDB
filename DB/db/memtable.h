#ifndef STORAGE_LEVELDB_DB_MEMTABLE_H_
#define STORAGE_LEVELDB_DB_MEMTABLE_H_



namespace Rabbitdb{
    class MemTable{
        public:
            // MemTables are reference counted. The initial reference count
            // is zero and the caller must call Ref() at least once.
            
        private:
            friend class MemTableIterator;
            friend class MemTableBackwardIterator;

            struct KeyComparator{
                const InternalKeyComparator comparator;
                explicit KeyComparator(const InternalKeyComparator& c):comparator(c){}
                int operator()(const char* a,const char* b)const;
            }

            typedef SkipList<const char*,KeyComparator> Table;

            ~MemTable(); // Private since only Unref() should be used to delete it

            KeyComparator comparator_;
            int refs_;
            Arena arena_;
            Table table_;
    };
}


#endif