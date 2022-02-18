## SSTable
### SSTable 的文件组织
```
Data Block 1 
Data Block 2  数据
.......
-------------------------
MetaBlock1     Meta信息
MetaBlock2
......
Meta Index Block
Index Block
Footer
```

逻辑上可分为两大块，数据存储区Data Block，以及各种Meta信息。

1.  文件中的K/V对是有序存储的，并且被划分到连续排列的Data Block里面，这些Data Block从文件头开始顺序存储。
2.  Meta Block。 存储的是Filter信息，比如Bloom过滤器，用于快速定位Key是否在data block中。
3.  MetaIndex Block. 是对Meta Block的索引，它只有一条记录，key是meta index的名字，也就是Filter的名字，
value为指向meta index的blockHandle；BlockHandle是一个结构体，成员offset_ 是Block在文件中的偏移，
成员size_是block的大小。
4. Index Block是对Data Block的索引，对于其中的每个记录，其key >= Data Block 最后一条记录的key，
同时 < 其他Data Block的第一条记录的key; value 是指向data index的blockHandle：
5. Footer,文件的最后，大小固定。
```
Metaindex_handle
index_handle
padding
Magic number 
```
- 成员metaindex_handle 指出了 meta index Block的起始位置和大小
- 成员index_handle 指出了 index block 的起始地址和大小
这两个字段都是BlockHandler对象，可以理解为索引的索引，通过Footer可以直接定位metaindex和index block。
再后面是一个填充区和magic number。

### block存储格式

- Data Block 是具体的K/V数据对存储区域，此外还有存储meta的metaIndex Block，存储data block索引信息的index block等等，他们都是以Block的方式存储的。
- 每个block有三部分构成：block data， type， crc32.
```
unsigned char[]   1 byte  4 byte
Block   data    | type  | crc32
```
- 类型type指明使用的是哪种压缩方式，当前支持none和snappy压缩

虽然block有好几种，但是block data都是有序的kv对，因此写入，读取BlockData的接口都是统一的，对于Block data的管理
也都是相同的。

对Block的写入，读取将在创建，读取SSTable时分析，知道了格式之后，其读取写入代码都是很直观的。

由于SSTable对数据的存储格式都是block，因此在分析sstable的读取和写入逻辑之前，我们先来分析下Leveldb对Block data
的管理。

Leveldb对Block data的管理是读写分离的，读取后的遍历查询操由Block类实现，Blockdata的构建则由blockbuilder类
实现。

#### block data的数据构建

BlockBuilder 对Key的存储是前缀压缩的，对于有序的字符串来讲，这能极大的减少存储空间。但是却增加
了查找的时间复杂度，为了兼顾查找效率，每隔K个key，leveldb就不使用前缀压缩，而是存储整个Key，这就是
重启点（restartpoint）。

在构建Block时，有参数Options::block_restart_interval 确定每隔几个Key就直接存储一个重启点key。

Block在结尾记录所有重启点的偏移，可以二分查找指定的key。Value直接存储在key的后面，无压缩。

对于一个k/v对，其在Block中的存储格式为：

- 共享前缀长度 shared_bytes: varint32
- 前缀之后的字符串长度 unshared_bytes:varint32
- 值的长度     value_length: varint32
- 前缀之后的字符串 key_delta: char[unshared_bytes]
- 值           value: char[value_length]

对于重启点，shared_bytes = 0

Block的结尾段格式是：

- > restarts：uint32[num_restarts]
- > num_restarts: uint32 // 重启点个数

元素restars[i] 存储的是block的第i个重启点的偏移。很明显第一个k/v对，总是第一个重启点，也就是restarts[0] = 0;

总体来看Block可分为k/v存储区和后面的重启点存储区两部分，其中k/v的存储格式如前面所讲，
可以看作4部分：
前缀压缩的key长度信息+value长度+key前缀之后的字符串+value
最后一个4byte为重启点的个数。
对Block的存储格式了解之后，对Block的构建和代码分析就是很直观的事情了。

#### Block的构建和读取

- BlockBuilder的接口
首先从Block的构建开始，这就是BlockBuilder类
```
void Reset(); // 重设内容，通常在Finish之后调用已构建新的Block
// 添加k/v，要求：reset()之后没有调用过Finish();Key > 任何已加入的key

void Add(const Slice& key,const Slice& value);
// 结束构建block，并返回指向block内容的指针
Slice Finsh(); // 返回Slice的生存周期：Builder的生存周期，or直到Reset被调用

size_t CurrentSizeEstimate()const;// 返回正在构建block的未压缩大小-估计值

bool empty() const {return buffer_.empty();}// 没有entry则返回true
```
主要成员变量如下：
```
std::string buffer_;   // block的内容
std::vector<uint32_t> restarts_; // 重启点
int                   counter_;  // 重启后生成的entry数
std::string           last_key_; // 记录最后添加的key
```

##### blockbuilder::Add()
- 调用Add函数向当前的Block中新加入一个k/v对{key,value}.函数处理逻辑如下：

1. Step1：
保证新加入的key > 已加入的任何一个key
2. step2:
如果计数器counter < options->block_restart_interval，则使用前缀压缩算法压缩key，否则
就把key作为一个重启点，无法压缩存储；
3. step3：
根据上面的数据格式存储k/v对，追加到buffer中，并更新block状态。


##### blockbuilder::Finish()
调用该函数完成Block的构建，很简单，压入重启点信息，并返回buffer_,设置结束标记finished_:
```
for(size_t i = 0; i < restarts_.size();i++){
    PutFixed32(&buffer_,restarts_[i]); 
}
PutFixed32(&buffer_,restarts_.size());//重启点数量
finished_ = true;
return Slice(buffer_);
```

##### blockbuilder::Reset()& 大小

还有Reset和CurrentSizeEstimate两个函数，Reset复位函数，清空各个信息；函数CurrentSizeEstimate返回
block的预计大小，从函数实现来看，应该在调用Finish之前调用该函数。


##### block类接口

