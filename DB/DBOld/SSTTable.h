//
// Created by Reeb Deve on 2021/12/13.
//

#ifndef RABBIT_LOGDB_SSTTABLE_H
#define RABBIT_LOGDB_SSTTABLE_H

#include <string>
#include <map>
#include "command.h"
namespace RabbitLog{
    /**
     * @brief SSTable 索引信息
     */
    class TableMetaInfo{
    public:
        /**
         * @brief 将数据写入到文件中
         * @param file
         */
        void writeToFile(RandomAccessFile file){

        }
        /**
         * @brief 从文件中读取元信息
         * @param file
         */
         static TableMetaInfo readFromFile(RandomAceessFile file){

         }

    private:
        /**
         * @brief 版本号
         */
        long int version;

        /**
         * @brief 数据区开始
         */
        long int dataStart;

        /**
         * @brief 数据区长度
         */
        long int dataLen;

        /**
         * @brief 索引区
         */
        long int indexStart;

        /**
         * @brief 索引区长度
         */
         long int indexLen;

         /**
          * @brief 分段大小
          */
         long int partSize;
    };
    /**
     * @brief 位置信息
     */
    class Position{
    private:
        /**
         * @brief 开始
         */
         long int start;
        /**
         * @brief 长度
         */
         long int len;
    };
    /**
     * @brief 排序字符串表
     */
     class SSTable {
     public:
         //Todo static std::string RW = "rw";
         // Logger
         /**
          * @brief 从内存表中构建sstable
          * @param filePath
          * @param partsize
          * @param index
          * @return
          */
         static SSTable createFromIndex(std::string filePath,
                                        int partsize,std::map<std::string,Command> index);
         /**
          * @brief 从文件中构建sstable
          * @param filePath
          * @return
          */
         static SSTable createFromFile(std::string filePath);
         /**
          * 从SSTable中查询数据
          * @param key
          * @return
          */
         Command query(std::string key);
         /**
          * @brief 关闭文件
          */
         void close();
     private:
         /**
          * @brief 将数据分区写入文件
          * @param partData
          */
         void writeDataPart(JSONObject partData)throw IOException;
         /**
          * @brief 从内存表转换为SSTable
          * @param index
          */
         void initFromIndex(std::map<std::string,Command> index);
         /**
          * @brief 从文件中恢复SSTable到内存
          */
         void restoreFromFile();
         /**
          * @brief
          * @param filePath 表文件路径
          * @param partsize 数据分区大小
          */
         SSTable(std::string filePath,int partsize);
         static std::string RW = "rw";
         /**
          * @brief 表索引信息
          */
         TableMetaInfo tableMetaInfo;
         /**
          * @brief 字段稀疏索引
          */
         map<std::string,Position> sparseIndex;
         /**
          * @brief 文件句柄
          */
         RandomAccessFile tableFile;
         /**
          * @brief 文件路径
          */
         std::string filePath;
         /**
          *
          * @param filePath
          * @param partSize
          */
         SSTable(std::string filePath,int partSize){

         }

     };
}

#endif //RABBIT_LOGDB_SSTTABLE_H
