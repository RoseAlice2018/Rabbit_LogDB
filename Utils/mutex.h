//
// Created by Reeb Deve on 2021/12/8.
//

#ifndef RABBIT_LOGDB_MUTEX_H
#define RABBIT_LOGDB_MUTEX_H
#include <thread>
#include <memory>
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>
#include <list>
#include <atomic>
namespace RabbitLog{
    /**
     * @brief 信号量
     */
    class Semaphore{
    public:
        /**
         * @brief 构造函数
         * @param[in] count 信号量值的大小
         */
         Semaphore(uint32_t count = 0);
        /**
         * @brief 析构函数
         */
         ~Semaphore();
        /**
         * @brief 获取信号量
         */
         void wait();
        /**
         * @brief 释放信号量
         */
         void notify();
    private:
        sem_t m_semaphore;
    };

    /**
     * @brief 互斥量
     */
     class Mutex {
     public:
         /**
          * @brief 构造函数
          */
         Mutex(){
             pthread_mutex_init(&m_mutex, nullptr);
         }
         /**
          * @brief 析构函数
          */
         ~Mutex(){
             pthread_mutex_destroy(&m_mutex);
         }
         /**
          * @brief 加锁
          */
          void lock(){
             pthread_mutex_lock(&m_mutex);
          }
         /**
          * @brief 解锁
          */
          void unlock(){
             pthread_mutex_unlock(&m_mutex);
          }
     private:
         /// mutex
         pthread_mutex_t m_mutex;
     };
     /**
      * @brief 自旋锁
      */
     class SpinLock{
     private:

     };
}

#endif //RABBIT_LOGDB_MUTEX_H
