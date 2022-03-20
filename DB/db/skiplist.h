#ifndef STORAGE_LEVELDB_DB_SKIPLIST_H_
#define STORAGE_LEVELDB_DB_SKIPLIST_H_


// Thread safety
// -------------
//
//  Writes require external synchronization,most likely a mutex.
//  Reads require a guarantee that the SkipList will not be destroyed
//  while the read is in progress. Apart from that, reads progress
//  without any internal locking or synchronization.
//
//  Invariants:
//  (1) Allocated nodes are never deleted until the SkipList is
//  destroyed.  This is trivailly guaranteed by the code since we
//  never delete any skip list nodes.
//
//  (2) The contents of a Node except for the next/prev pointers are
//  immutable after the Node has been linked into the SkipList.
//  Only Insert() modifies the list, and it is careful to initialize
//  a node and use release-stores to publish the nodes in one or
//  more lists.
//
// ... prev vs. next pointer ordering ...

#include <atomic>
#include <cassert>
#include <cstdlib>



namespace Rabbitdb{
    template<typename Key,class Comparator>
    class SkipList{
        private:
            struct Node;
        
        public:
            // Create a new SkipList object that will use "cmp" for comparing keys,
            // and will allocate memory using "*arena". Objects allocated in the  arena
            // must remain allocated for the lifetime of the skiplist object.
            explicit SkipList(Comparator cmp,Arena* arena);

            SkipList(const SkipList&) = delete;
            SkipList& operator=(const SkipList&) = delete;

            // Insert key into the list.
            // REQUIRES: nothing that compares equal to key is currently in the list.
            void Insert(const Key& key);

            // Returns true iff an entry that compares equal to key is in the list.
            bool Contains(const Key& key) const;

            // Iteration over the contents of a skip list
            class Iterator{
                public:
                // Initialize an iterator over the specified list.
                // The returned iterator is not valid.
                explicit Iterator(const SkipList* list);

                // Returns true iff the iterator is positioned at a valid node.
                bool Valid() const;

                // Returns the key at the current position.
                // REQUIERS: Valid()
                const Key& key() const;

                // Advances to the next position
                // REQUIERS: Valid()
                void Next();

                // Advances to the previous position
                // REQUIERS: Valid()
                void Prev();

                // Advance to the first entry with a key >= target
                void Seek(const Key& target);

                // Position at the first entry in list.
                // Final state of iterator is Valid() iff list is not empty.
                void SeekToFirst();

                // Position at the last entry in list.
                // Final state of iterator is Valid() iff list is not empty.
                void SeekToLast();

                private:
                    const SkipList* list_;
                    Node* node_;
                    //Intentionally copyable
            };      

            private:
                enum { kMaxHeight = 12 };

                inline int GetMaxHeight() const{

                }

                Node* NewNode(const Key& key,int height);
                int RandomHeight();
                bool Equal(const Key& a,const Key& b)const {return (compare_(a,b) == 0);}

                // Return true if key is greater than the data stored in "n"
                bool KeyIsAfterNode(const Key& key,Node* n)const;

                // Return the earliest node that comes at or after key.
                // Return nullptr if there is no such node.
                //
                // If prev is non-null,fills prev[level] with pointer to previous
                // node at "level" for every level in [0..max_height_-1];
                Node* FindGreaterOrEqual(const Key& key,Node** prev)const;

                // Return the lastest node with a key < key.
                // Return head_ if there is no such node.
                Node* FindLessThan(const Key& key)const;

                // Return the last node in the list.
                // Return head_ if list is empty.
                Node* FindLast() const;

                // Immutable after construction
                Comparator const compare_;
                Arena* const arena_;    // Arena used for allocations of nodes

                Node* const head_;

                // Modified only by Insert(). Read racily by readers, but stable
                // values are ok.
                std::atomic<int> max_height_;   //Height of the entire list

                // Read/written only by Insert()
                Random rnd_;
    };

    // Implementation details follow
    template <typename Key,class Comparator>
    struct SkipList<Key,Comparator>::Node{
        explicit Node(const Key& k):  key(k){}

        Key const key;

        // Accessors/mutators for links. Wrapped in methods so we can
        // add the appropriate barriers as necessary.
        Node* Next(int n){
            assert(n >= 0);
            // Use an 'acquire load' so that we observe a fully initialized
            // version of the returned Node.
            return next_[n].load(std::memory_order_acquire);         
        }
        void SetNext(int n, Node* x){
            
        }
    };
    
}



#endif
