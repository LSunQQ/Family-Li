# 论文阅读与前期工作总结

姓名：李僖哲，李文浩，李赛尉，李济邦，李宸

学号：17343064，17343063，17343062，17343061,17343060



## 前期工作

### 使用示意图展示普通文件IO方式(fwrite等)的流程，即进程与系统内核，磁盘之间的数据交换如何进行？为什么写入完成后要调用fsync？

![](/images-h1/2.png)



![](/images-h1/1.png)



#### 为什么写入完成后要调用fsync？

由图可见，使用fsync()的原因是要把缓存中的数据刷新到磁盘里面。



### 简述文件映射的方式如何操作文件。与普通IO区别？为什么写入完成后要调用msync？文件内容什么时候被载入内存？



```c
HANDLE CreateFile(LPCTSTR lpFileName,
DWORD dwDesiredAccess,
DWORD dwShareMode,
LPSECURITY_ATTRIBUTES lpSecurityAttributes,
DWORD dwCreationDisposition,
DWORD dwFlagsAndAttributes, 
HANDLE hTemplateFile);
```

函数CreateFile()用来创建、打开文件，在处理内存映射文件时，该函数来创建/打开一个文件内核对象，在调用该函数时需要根据是否需要数据读写和文件的共享方式来设置参数dwDesiredAccess和dwShareMode。



```c
HANDLE CreateFileMapping(HANDLE hFile,
LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
DWORD flProtect,
DWORD dwMaximumSizeHigh,
DWORD dwMaximumSizeLow,
LPCTSTR lpName);
```

CreateFileMapping()函数创建一个文件映射内核对象，通过参数hFile将CreateFile()的返回值映射到进程地址空间。



```c
LPVOID MapViewOfFile(HANDLE hFileMappingObject,
DWORD dwDesiredAccess,
DWORD dwFileOffsetHigh,
DWORD dwFileOffsetLow,
DWORD dwNumberOfBytesToMap);
```

MapViewOfFile()函数负责把文件数据映射到进程的地址空间。



```c
SYSTEM_INFO sinf;
GetSystemInfo(&sinf);
DWORD dwAllocationGranularity = sinf.dwAllocationGranularity;
```

参数dwNumberOfBytesToMap指定了数据文件的映射长度。



```c
BOOL UnmapViewOfFile(LPCVOID lpBaseAddress);
```

参数lpBaseAddress指定了返回区域的基地址，必须将其设定为MapViewOfFile()的返回值。



内存映射文件只指将整个文件或者文件的一部分映射到内存中，这个文件就可以像内存数组一样访问。内存映射文件会用到FileChannel类，对文件的读写都转化成了对内存缓冲区的操作。内存映射文件的操作：首先是建立文件通道，然后调用map方法获取映射的内存缓冲区，最后操作内存缓冲区即可（缓冲区中的更改会在适当的时候和通道关闭时写回存储设备）。

```c
static FileChannel open(Path path, OpenOption... options)    //也使用Files类介绍中提到的StandardOpenOption
static FileChannel open(Path path, Set<? extends OpenOption> options, FileAttribute<?>... attrs)
long size()     //文件大小
long position()   //返回文件指针位置
void position(long pos)    //设置文件指针位置
void force(boolean metaData)    //强制将文件的变更同步到存储设备
FileChannel truncate(long size)     //截断
int read(ByteBuffer dst)...　　  //从通道读入内容到缓冲区
int write(ByteBuffer src)...     //将缓冲区内容写入通道
long transferTo(xxx)
long transferFrom(xxx)
MappedByteBuffer map(MapMode, long startposition, long size)   MapMode.READ_ONLY|READ_WRITE|PRIVATE
void close()     //关闭通道，如果有可写的内存映射，其中的更改会写回存储设备
```



#### 内存映射步骤：

用open系统调用打开文件, 并返回描述符fd
用mmap建立内存映射, 并返回映射首地址指针start
对映射(文件)进行各种操作,
用munmap(void *start, size_t lenght)关闭内存映射
用close系统调用关闭文件fd



#### 与普通IO的区别：

常规IO文件操作为了提高读写效率和保护磁盘，使用了页缓存机制。这样造成读文件时需要先将文件页从磁盘拷贝到页缓存中，由于页缓存处在内核空间，不能被用户进程直接寻址，所以还需要将页缓存中数据页再次拷贝到内存对应的用户空间中。这样，通过了两次数据拷贝过程，才能完成进程对文件内容的获取任务。写操作也是一样，待写入的buffer在内核空间不能直接访问，必须要先拷贝至内核空间对应的主存，再写回磁盘中（延迟写回），也是需要两次数据拷贝。


而使用mmap操作文件中，创建新的虚拟内存区域和建立文件磁盘地址和虚拟内存区域映射这两步，没有任何文件拷贝操作。而之后访问数据时发现内存中并无数据而发起的缺页异常过程，可以通过已经建立好的映射关系，只使用一次数据拷贝，就从磁盘中将数据传入内存的用户空间中，供进程使用。


总而言之，常规文件操作需要从磁盘到页缓存再到用户主存的两次数据拷贝。而mmap操控文件，只需要从磁盘到用户主存的一次数据拷贝过程。说白了，mmap的关键点是实现了用户空间和内核空间的数据直接交互而省去了空间不同数据不通的繁琐过程。因此mmap效率更高。  



#### 写完之后调用msync()函数的原因：

进程在映射空间对共享内容的改变并不直接写回到磁盘文件中，可以通过调用msync()函数来实现磁盘文件内容与共享内存区中的内容一致,即同步操作。



#### 文件内容什么时候被载入内存：

打开文件之后就用mmap()建立内存映射，将文件内容载入内存缓冲区，之后的操作都在内存缓冲区里面就可以了。



### 参考[Intel的NVM模拟教程](https://software.intel.com/zh-cn/articles/how-to-emulate-persistent-memory-on-an-intel-architecture-server)模拟NVM环境，用fio等工具测试模拟NVM的性能并与磁盘对比（关键步骤结果截图）。

输出快照

![](/images-h1/输出快照.png)



写前（未挂载）

![](/images-h1/写前.png)



写后（未挂载）

![](/images-h1/写后.png)



挂载

![](/images-h1/挂载.png)

挂载到/mnt/pemedir下



用fio工具测试磁盘

IOPS = 129，BW = 2077KiB/s

![](/images-h1/对硬盘.png)



用fio工具测试模拟NVM

IOPS = 1435K，BW =21.9GiB/s

![](/images-h1/对NVM1.jpg)
![](/images-h1/对NVM2.jpg)





### 使用[PMDK的libpmem库](http://pmem.io/pmdk/libpmem/)编写样例程序操作模拟NVM（关键实验结果截图，附上编译命令和简单样例程序）。

1. 对于PMDK库的编译安装

首先通过git clone https://github.com/pmem/pmdk指令下载pmdk包，然后在README中看到其需要安装的库，如下图：

![](/images-h1/one.png)



通过对于相关博客<https://www.jianshu.com/p/bba1cdf01647>的参考下对于其进行安装。但是在安装过程中对于autoconf与pkg-config的安装都无太大问题，可对于libndctl安装过程中总是出现报错，而其中uuid-devel与json-c-devel在安装过程中找不到包，导致对于libndctl库的安装无法进行。

![](/images-h1/two.png)



后面发现可以直接使用apt-get install libndctl-dev进行安装，可是还是在pmdk目录下进行make的时候，还是说我们没有安装libndctl库，但是我们输入sudo apt search libndctl之后发现它其实已经安装好了

![](/images-h1/three.png)



后来尝试了重启一下之后输入make不知道为什么就成功了，输出如下：

![](/images-h1/o.jpg)



再make install 一下

![](/images-h1/t.jpg)



生成红色圈里面的东西

![](/images-h1/s.jpg)





这是我们使用的测试代码

![](/images-h1/代码.jpg)





用vim打开simple_copy.vcxproj，输出如下：

![](/images-h1/a.jpg)



然后用以下命令复制：

![](/images-h1/q.png)



用vim打开simple_copy.vcxproj.filter ，输入如下：

![](/images-h1/f.jpg)



## 论文阅读

### 总结一下本文的主要贡献和观点(500字以内)(不能翻译摘要)。

(总结：本文工作的动机背景是什么，做了什么，有什么技术原理，解决了什么问题，其意义是什么)

由于新一代的存储结构SCM（储存级储存器）的出现，计算机的存储结构将会有一次大变革，新的存储器是介于DRAM和传统存储装置之间的装置，它的读写速度比传统储存器更快，虽然比不上DRAM与SRAM，但它的成本比这两者都低，而且是非易失性的，这就使读写速度限制而产生的传统储存结构可以被颠覆，本文根据SCM的特点，而创造性地提出一种新型的B+树储存结构，称为FPTree。虽然我们强调SCM具有快速的存取特性，很像DRAM，但实际上它仅仅读速度与DRAM相当（还是会慢大约一个数量级），写速度则相差10倍到100倍以上。并且SCM还具有一个作为NVM设备的致命缺陷，即写次数有限，写几百万次时会写穿，造成永久失效的问题。因此，对于写次数多的应用而言，SCM未必是个好的选择。 因此，DRAM现阶段是不可能也不应该被淘汰的。但是否可以设计一种混合架构内存，使DRAM和SCM联系起来？本文就根据这个思路，结合了DRAM的快速存取特点和SCM的新特性，将传统B+树的叶节点存储在SCM中，将内节点放置在DRAM中，并在恢复时重建，这既可以保持B+树通过DRAM的高速查找与读写，也利用了SCM的非易失性的特点使B+树有了持久性储存的优势，通过新旧技术的组合，探索了新技术，验证了SCM作为下一代储存结构的优势，也证明了SCM具有超越替代DRAM的潜力。文中通过详细的程序设计，使FPTree具有许多特殊的性质，解决了很多由于硬件限制而未完成的问题，大量的数据测试也证明了这种FPTree在性能，持久性，并发查找，快速恢复和扩展性等方面具有传统结构无法比拟的成倍提高的优势。



### SCM硬件有什么特性？与普通磁盘有什么区别？普通数据库以页的粒度读写磁盘的方式适合操作SCM吗？

1. 非易失
2. 极短的存取时间（DRAM-like）
3. 每比特价格低廉（Disk-like）
4. 固态，无移动区（SSD-like）

它与普通磁盘最大的区别就是读写速度的不同，可以理解为一种速度更快的SSD。

我认为不适合，因为普通数据库，用页粒度读写磁盘是为了提高读写效率，但SCM已经从硬件方面解决了这个问题，更应该考虑的应该是读写是对空间的充分利用的问题，所以普通数据库以页的粒度读写磁盘的方式并不适合操作SCM。



### 操作SCM为什么要调用CLFLUSH等指令？

(写入后不调用，发生系统崩溃有什么后果)

因为SCM是一种NVM，通过CLFLUSH（Cache Line Flush，缓存行刷回）能够把指定缓存行（Cache Line）从所有级缓存中淘汰，若该缓存行中的数据被修改过，则将该数据写入主存。

这里就要涉及到关于现代计算机系统的一个基本事实：内存级reordering。

这里查找资料可以知道：

1. 什么叫reordering呢？简单来说就是数据乱序输出。

2. 从哪里输出到哪里呢？数据会从cache乱序输出到memory。



### FPTree的指纹技术有什么重要作用？

指纹是连续放置在叶子的第一个缓存行大小的片段中的内嵌键的单字节哈希值， FPTree使用未分类的叶子和叶子内的位图，这样就可以在叶中线性地成为有效的key。通过首先扫描指纹，我们能够将叶内探测键的数量限制在平均情况之下，从而显着提高性能。



### 为了保证指纹技术的数学证明成立，哈希函数应如何选取？

（哈希函数生成的哈希值具有什么特征，能简单对键值取模生成吗？）

哈希函数生成的哈希值应尽量均匀分布，对键简单地值取模生成大概率是没有用的，哈希函数的实现可能要利用到分析键值的概率分布。



### 持久化指针的作用是什么？与课上学到的什么类似？

当一个程序或者系统重新启动时，会使用新的地址空间，这样原本储存的旧的虚拟指针就无效了，而持久化指针在程序结束或故障期间持续有效，所以它用于在重启时刷新易失性指针。

这与项目中在程序或系统退出时将必要的信息储存在.txt文件中类似。
