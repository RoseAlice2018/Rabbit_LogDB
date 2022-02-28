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

对Block的读取是由类block完成的。
block只有两个函数接口，通过Iterator对象，调用者可以遍历访问block所有的存储的kv对
```
size_t size() const {return size_;}
Iterator* NewIterator(const Comparator* comparator);

const char* data_;  // block数据指针
size_t size_;       // block数据大小
uint32_t restart_offset_; // 重启点数组在data_中的偏移
bool owned_;        // data_[]是否是block拥有的
```

##### block初始化

block的构造函数接受一个Blockcontents对象 contents初始化，
BlockContents是一个有着3个成员的结构体。
```
- data = Slice();
- cachable = false;// 无cache
- heap_allocated = false;// 非heap分配
```
根据contents为成员赋值
```
data_ = contents.data.data(),
size_ = contents.data.size(),
owned_ = contents.heap_allocated;
```
然后从data中解析出重启点数组，如果数据太小，或者重启点计算错误，就设置size_ = 0,
表明该block data解析失败。

##### Block::Iter
这是一个用以遍历Block内部数据的内部类，它继承了Iterator接口。
函数NewIterator返回Block::Iter对象：
```
return new Iter(cmp,data_,restart_offset_,num_restarts);
```
下面我们就分析Iter的实现。
主要成员变量有：
```
const Comparator* constcomparator_; // Key比较器
const char* const data_;            // block内容
uint32_t const restarts_;           // 重启点（uint32数组）在data中的偏移
uint32_t const num_restarts_;       // 重启点个数
uint32_t current_;                  // 当前entry在data中的偏移，
```
下面来看看对Iterator接口的实现

###### Next()函数 
直接调用private函数ParseNextKey()跳到下一个k/v对，函数实现如下
1. Step1
**跳到下一个entry，其位置紧邻在当前value_之后。如果已经是最后一个entry了，返回false，标记current_为invalid。**
2. Step2
**解析出entry，解析出错则设置错误状态，记录错误并返回false。解析成功则根据信息组成key和value，并更新重启点index**

###### Prev()函数
Previous操作分为两步：首先回到current_之前的重启点，然后再向后直到current_,实现如下：
1. Step1
首先向前回跳到在current_前面的那个重启点，并定位到重启点的kv对开始位置。
2. Step2
从重启点位置开始向后遍历，直到遇到original前面的那个kv对。

###### Seek()函数
1. step1
二分查找，找到key < target 的最后一个重启点，典型的二分查找算法。
2. step2
找到后，跳转到重启点，其索引由left指定，这是前面二分查找到的结果。如前面所分析的，value_指向重启点的
地址，而size_指定为0，这样ParseNextKey函数将会取出重启点的kv值。
```
SeekToRestartPoint(left);
```
3. step3
自重启点线性向下，直到遇到key >= target 的kv对。
```
while(true){
    if(!ParseNextKey())return;
    if(Compare(key_,target)>=0)return;
}
```


上面就是Block::Iter的全部实现逻辑，这样Block的创建和读取遍历都已经分析完毕。


### 创建SSTable文件

了解了sstable文件的存储格式，以及Data Block的组织，下面就可以分析如何创建sstable文件。
相关代码在table_builder.h/.cc 以及block_builder.h/.cc(构建block)中。

#### TableBuilder类

构建sstable文件的类是TableBuilder,该类提供了几个有限的方法可以用来添加kv对，Flush到文件中等等，它依赖于
blockbuilder来构建block。

- TableBuilder的几个接口如下：
1. void Add(const Slice& key,const Slice& value)，向当前正在构建的表添加新的{key,value}对，要求
根据Option指定的Comparator，key必须位于所有前面添加的key之后。
2. void Flush(),将当前缓存的kv全部flush到文件中，一个高级方法，大部分client不需要直接调用该方法；
3. void Finish(),**结束表的**的构建，该方法被调用后，将不会再使用传入的WritableFile；
4. void Abandon(),结束表的构建，并丢弃当前缓存的内容，该方法被调用以后，将不再会使用传入的writablefile；（
只是设置closed为true，无其他操作）
5. 一旦**Finish()/Abandon()**方法被调用，将不能在再次执行Flush或者Add操作。

TableBuilder只有一个成员变量Rep* rep_,实际上Rep结构体的成本就是TableBuilder
所有的成员变量。
Rep简单解释下成员的含义：
```
Options options;    // data block的选项
Options index_block // index block的选项
WritableFile* file  // sstable文件
uint64_t offset;
// 要写入data block在sstable文件中的偏移，初始0
Status status;      // 当前状态-初始ok
BlockBuilder data_block; // 当前操作的data block
BlockBuilder index_block; // sstable的index block
std::string last_key; 
int64_t num_entries;      // 当前datablock的个数，初始0
bool closed;            // 调用了Finish() or Abandon(), 初始false
FilterBlockBuilder* filter_block;
//根据filter数据快速定位key是否在block中
bool pending_index_entry;   // 见下面的Add函数，初始false
BlockHandle pending_handle; // 添加到index block的data block的信息
std::string compressed_output; // 压缩后的data block ，临时存储，写入后即被清空
```
- 添加kv对
这是通过方**Add(const Slice& key,const Slice& value)**完成的，没有返回值。
下面分析下函数的逻辑：
1. step1
首先保证文件没有close，也就是没有调用过Finish/Abandon,以及保证当前status是ok的；
如果当前有缓存的kv对，保证新加入的key是最大的。
```
Rep* r = rep_;
assert(!r->closed);
if(!ok())return;
if(r-> num_entries > 0)
{
    assert(r->options.comparator->Compare(key,Slice(r->last_key))>0);
}
```
2. step2
如果标记r->pending_index_entry为true,表明遇到下一个data block的第一个kv，根据key调整
r->last_key，这是通过Comparator的FindShortestSeparator完成的。
```
if(r->pending_index_entry)
{
    assert(r->data_block.empty());
    r->options.comparator->FindShortestSeparator(&r->last_key,key);
    std::string handle_encoding;
    r->pending_handle.EncodeTo(&handle_encoding);
    r->index_block.Add(r->last_key,Slice(handle_encoding));
    r->pending_index_entry = false;
}
```
接下来将pending_handle加入到index block中{r->last_key,r->pending_handle's string}.
最后将r->pending_index_entry设置为false。

值得讲讲pending_index_entry这个标记的意义：

直到遇到下一个datablock的第一个key时，我们才为上一个datablock生成index entry,
我们才为上一个datablock生成index entry,这样的**好处**是：
可以为index使用较短的key;比如上一个datablock最后一个kv的key是"the quick brown fox",
其后继data block的第一个key是"the who",我们就可以用一个较短的字符串"the r"作为
上一个data block的index block entry的key。

简而言之，就是在开始**下一个datablock**时，Leveldb才将上一个data block加入到index block中。
标记pending_index_entry就是干这个用的，对应data block的index entry信息就保存在
（BlockHandle）pending_handle.

3. step3
如果filter_block不为空，就把key加入到filter_block中。
```
if( r-> filter_block != NULL)
{
    r-> filter_block -> AddKey(key);
}
```

4. step4
设置r->last_key = key,将（key,value)添加到r->data_block中，并更新entry数。
```
r->last_key.assign(key.data(),key.size());
r->num_entries++;
r->data_block.Add(key,value);
```

5. step5
如果data block的个数超过限制，就立即Flush到文件中。


#### Flush 文件

直接见代码

#### WriteBlock函数

在Flush文件时，会调用WriteBlock函数将datablock写入到文件中，该函数同时还设置**datablock**
的**index entry**信息。
```
void WriteBlock(BlockBuilder* block,BlockHandle* handle)
```
该函数做些预处理工作，序列化要写入的data block，根据需要压缩数据，真正的写入逻辑是在
**WriteRawBlock**函数中。下面分析该函数的处理逻辑。

1. step1

获得block的序列化数据Slice，根据配置参数决定是否压缩，以及根据压缩格式压缩数据内容。对于Snappy压缩，如果压缩率太低<12.5%，还是作为未压缩内容存储。

2. step2

将data内容写入到文件，并重置block成初始化状态，清空compressedoutput。


#### WriteRawBlock函数
在WriteBlock把准备工作都做好后，就可以写入到sstable文件中了。来看函数原型:
```
void WriteRawBlock(const Slice& data, CompressionType, BlockHandle*handle);
```

#### Finish函数

调用Finish函数，表明调用者将所有已经添加的k/v对持久化到sstable，并关闭sstable文件。

1. step1

首先调用Flush，写入最后的一块data block，然后设置关闭标志closed=true。表明该sstable已经关闭，不能再添加k/v对。

2. step2

写入filter block到文件中。

3. step3

写入meta index block到文件中。

如果filterblock不为NULL，则加入从"filter.Name"到filter data位置的映射。通过meta index block，可以根据filter名字快速定位到filter的数据区。

4. step4

写入index block，如果成功Flush过data block，那么需要为最后一块data block设置index block，并加入到index block中。

5. step5
写入Footer

