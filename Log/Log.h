//
// Created by Reeb Deve on 2021/12/5.
//

#ifndef RABBIT_LOGDB_LOG_H
#define RABBIT_LOGDB_LOG_H


#include <stdint.h>

namespace RabbitLog{
    class LogLevel{
    public:
        /**
         * @brief 日志级别枚举
         */
         enum Level{
             //未知级别
             UNKNOW = 0,
             //DEBUG级别
             DEBUG = 1,
             //INFO级别
             INFO = 2,
             //WARN级别
             WARN = 3,
             //ERROR级别
             ERROR = 4,
             //FATAL级别
             FATAL = 5
         };
         /**
          * @brief 将日志级别转成文本输出
          * @param[in] str 日志级别文本
          */
          static const char* ToString(LogLevel::Level level);

          /**
           * @brief 将文本转换成日志级别
           * @parm[in] str 日志级别文本
           */
           static LogLevel::Level FromString(const std::string& str);
    };


    /**
     * @brief 日志事件
     */
    class LogEvent{
    private:
        /// 文件名
        const char* m_file = nullptr;
        /// 行号
        int32_t m_line = 0;
        /// 程序开始到现在的毫秒数
        uint32_t m_elapse = 0;
        /// 线程ID
        uint32_t m_threadId = 0;
        /// 协程ID
        uint32_t m_fiberId = 0;

    };


    class Log{
    private:
        LogLevel level; //日志级别

    };
}

#endif //RABBIT_LOGDB_LOG_H
