#ifndef STORAGE_LEVELDB_INCLUDE_STATUS_H_
#define STORAGE_LEVLEDB_INCLUDE_STATUS_H_

#include "ldb/slice.h"
namespace Rabbitdb{
    class Status{
        public:
            // Create a success status .
            
        private:
        enum Code{
            kOk = 0,
            kNotFound = 1,
            KCorruption = 2,
            kNotSupported = 3,
            kInvalidArgument = 4,
            kIOError = 5
        };

        Code code()const{
            return (state_ == nullptr)?kOk : static_cast<Code>(state_[4]);
        }
        Status(Code code,const Slice& msg,const Slice& msg2);
        static const char* CopyState(const char* s);

        const char* state_;
    };
}


#endif