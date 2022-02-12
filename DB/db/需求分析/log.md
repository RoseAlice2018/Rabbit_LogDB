## Log
- 所有的写操作都必须先成功的append到操作日志中，然后再更新内存memtable。这样做有两点：
1. 可以将随机的写IO变成append，极大的提高了磁盘速度。
2. 防止在节点down机导致内存数据的丢失。
Log格式：
```
The log file contents are a sequence of 32KB blocks.
The only exception is that the tail of the file may contain a partial block.
Each block consists of a sequence of records:
block := record* trailer?
record := 
checksum: uint32    //crc32c of type and data[];little-endian
length:   uint16    //little-endian
type:     uint8     //One of FULL,FIRST,MIDDLE,LAST
data:     uint8[length];
```
- Leveldb把日志文件切分成大小为32KB的连续block块，block由连续的log record组成，log record的格式
为：checksum length type data

- Log Type
1. FULL = 1 FULL类型表明该log record包含了完整的user record.
2. FIRST = 2 FIRST 类型说明该log record是user record 的第一条log record
3. MIDDLE = 3 MIDDLE 说明user record 中间的log record
4. LAST = 4          说明user record 最后一条的log record

### 写日志

#### Writer 类分析
##### Writer类成员
-----------成员变量-------------
- WritableFile* dest_
- int block_offset_
- uint32_t type_crc_[kMaxRecordType + 1]
----------成员函数-------------
+ Status AddRecord(const Slice& slice)()
##### Interface WritableFile
- Append()
- Close()
- Flush()
- Sync()

##### 写入流程
- 首先取出slice的字符串指针和长度，初始化begin=true，表明这是第一条log record
```
const char* ptr = slice.data();
size_t left = slice.size();
bool begin = true;
```
然后进入一个do{}while循环，直到写入出错，或者成功写入全部数据，如下
1. step1 
首先查看当前block是否<7,如果<7则不惟，并重置block偏移
```
dest_->Append(Slice("\x00\x00\x00\x00\x00\x00",leftover));
block_offset_ = 0;
```
