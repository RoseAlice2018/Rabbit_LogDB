//
// Created by Reeb Deve on 2021/12/14.
//

#include <../DB/LSMKVDB.h>
#include <iostream>
int main()
{
    RabbitLog::DB db = new RabbitLog::DB("/Users/wty/kvstore/db/",4,3);
    for(int i=0;i<100;i++)
    {
        db.set(i+" ",i+" ");
    }
    for(int i=0;i<100;i++)
    {
        if(i+" " == db.get(i+ " "))
            ;
        else
            std::cout<<i<<" false"<<endl;
    }
    for(int i=0;i<100;i++)
    {
        db.rm(i+ " ");
    }
    for(int i=0;i<100;i++)
    {
        if(db.get(i+" ")==NULL)
            ;
        else
            std::cout<<i<<" false"<<endl;
    }
}