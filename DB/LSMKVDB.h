//
// Created by Reeb Deve on 2021/12/13.
//

#ifndef RABBIT_LOGDB_LSMKVDB_H
#define RABBIT_LOGDB_LSMKVDB_H

#include <string>
#include <map>
#include <list>
#include "command.h"
#include "SSTTable.h"
namespace RabbitLog{
    /**
     * @brief 数据库
     */
    class DB{
    public:
        /**
         * @brief 保存数据
         * @param[in] key 关键字
         * @param[in] value 值
         */
        void set(std::string key,std::string value);
        /**
         * @brief 查询数据
         * @param[in] key 关键字
         */
        std::string get(std::string key);
        /**
         * @brief 删除数据
         * @param[in] key 值
         */
        void rm(std::string key);

        /**
         * @brief 初始化
         * @param dataDir  数据目录
         * @param storeThreshold 存储阈值
         * @param partSize 分区大小
         */
        DB(std::string dataDir,int storeThreshold,int partSize);


    private:
        /**
         * @brief 从暂存日志中恢复数据
         * @param wal
         */
        void restoreFromWal(RandomAccessFile wal);

        /**
         * @brief 切换内存表，新建一个内存表，老的暂存起来
         */
        void switchIndex();

        /**
         * @brief 保存数据到SsTable
         */
        void storeToSsTable();





        /**
         * @brief 内存表
         */
        std::map<std::string ,Command> index;

        /**
         * @brief 不可变内存表，用于持久化内存表中时暂存数据
         */
        std::map<std::string, Command> immutableIndex;
        /**
         * @brief ssTable列表
         */
        std::list<SSTable> ssTables;
        /**
         * @brief 数据目录
         */
        std::string dataDir;

        /**
         * @brief 读写锁
         */
        ReadWriteLock indexLock;
        /**
         * @brief 持久化阈值
         */
        int storeThreshold;
        /**
         *@brief 数据分区大小
         */
        int PartSize;
        /**
         * @brief 暂存数据的日志句柄
         */
        RandomAccessFile wal;
    };
}




#endif //RABBIT_LOGDB_LSMKVDB_H
