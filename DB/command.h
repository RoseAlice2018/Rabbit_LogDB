//
// Created by Reeb Deve on 2021/12/13.
//

#ifndef RABBIT_LOGDB_COMMAND_H
#define RABBIT_LOGDB_COMMAND_H

#include <string>
namespace RabbitLog{
    /**
     * @brief 命令类型
     */
    enum CommandType{
        /**
         * @brief 保存命令
         */
        SET,
        /**
         * @brief 删除命令
         */
        RM
    };
    /**
     * @brief Command 基类
     */
    class Command{
    public:
        /**
         * @brief 获取数据Key
         * @return
         */
        virtual String getKey()=0;
    };
    /**
     * @brief 抽象命令
     */
    class AbstractCommand:public Command{
    public:
        AbstractCommand(CommandType type){
            this->type = type;
        }
    //Todo：添加json tostring
    private:
        CommandType type;
    };

    /**
     * @brief 删除命令
     */
    class RmCommand: public  AbstractCommand{
    public:
        RmCommand(std::string key): AbstractCommand(RM){
            this->key = key;
        }
    private:
        /**
         * @brief 数据Key
         */
        std::string  key;
    };

    /**
     * @brief 保存命令
     */
    class SetCommand: public AbstractCommand{
    public:
        SetCommand(std::string key,std::string value): AbstractCommand(SET){
            this->key = key;
            this->value = value;
        }
    private:
        /**
         * @brief 数据key
         */
        std::string key;
        /**
         * @brief 数据值
         */
        std::string value;
    };


}

#endif //RABBIT_LOGDB_COMMAND_H
