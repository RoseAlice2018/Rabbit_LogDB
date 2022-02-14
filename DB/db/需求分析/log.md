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
2. step2
计算block剩余大小，以及本次log record可写入数据长度
```
const size_t avail = kBlockSize - block_offset_ - kHeaderSize;
const size_t fragment_length = (left < avail) ? left : avail;
```
3. step3
判断log type
```
RecordType type;
const bool end = (left == fragment_length);// 两者相等 表明 left < avail 足以在剩余block中写完
if(begin && end) type = kFullType;
else if(begin)   type = kFirstType;
else if(end)     type = kLastType;
else             type = kMiddleType;
```
4. step4
调用EmitPhysicalRecord函数，append日志；
并更新指针，剩余长度和begin标记
```
s = EmitPhysicalRecord(type,ptr,fragment_length);
ptr += fragment_length;
left -= fragment_length;
begin = false;
```
- EmitPhysicalRecord函数
StatusWriter::EmitPhysicalRecord(RecordType t,const char* ptr,size_t n)
参数ptr为用户record数据，参数n为record长度，不包含log header。
1. step1
计算header，并Append到log文件，header共7byte格式为：
```
|CRC32(4Byte) | payload length lower + high (2 byte) | type (1 byte)|
char buf[kHeaderSize];
buf[4] = static_cast<char>(n & 0xff);
buf[5] = static_cast<char>(n >> 8);
buf[6] = static_cast<char>(t); // 计算record type和payload的CRC校验值
uint32_t crc = crc32c ::Extend(type_crc[t],ptr,n);
crc = crc32c::Mask(crc);
EncodeFixed32(buf,crc);
dest_->Append(Slice(buf,kHeaderSize));
```
2. step2
写入payload，并flush，更新block当前偏移
```
s = dest_->Append(Slice(ptr,n));
s = dest_->Flush();
block_offset_ += kHeaderSize + n;
```


### 读日志

#### Reader类
Reader主要用到两个接口，一个是汇报错误的Reporter，另一个是log文件读取类SequentialFile。

- Reporter的接口
```
void Corruption(size_t bytes,const Status& status);
``` 
- SequentialFile 接口
```
Status Read(size_t n,Slice* result,char* scratch);
Status Skip(uint64_t n);
```
- Reader 类
```
- SequenctialFile* const file_
- Reporter* const reporter_
----------------------------------
+ bool ReadRecord(Slice* record,string* scratch)()
+ uint64_t LastRecordOffset()()
```
Reader类有几个成员变量，需要注意
```
bool eof_ ; // 上次Read()返回长度 < kBlockSize,暗示到了文件结尾EOF
uint64_t last_record_offset_; // 函数ReadRecord返回的上一个record的偏移
uint64_t end_of_buffer_offset_; // 当前的读取偏移
uint64_t const initial_offset_; // 偏移，从哪里开始读取第一条record
Slice    buffer_;               // 读取的内容
```

#### 日志读取流程

Reader只有一个接口，那就是ReadRecord，下面来分析这个函数。

1. 
根据初始的initial offset跳转到调用者指定的位置，开始读取日志文件，跳转就是直接调用SequentialFile的Seek接口。

另外，需要先调整调用者传入的initialoffset参数，调整和跳转逻辑在SkipToInitialBlock函数中。
```
if (last_record_offset_ < initial_offset_)
{
    // 当前偏移 < 指定的偏移 ，需要
}
```