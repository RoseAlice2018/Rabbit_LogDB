cmake_minimum_required(VERSION 3.20)
project(Rabbit_LogDB)

set(CMAKE_CXX_STANDARD 14)

include_directories(DB)
include_directories(Log)
include_directories(Utils)

add_executable(Rabbit_LogDB
        DB/command.h
        DB/LSMKVDB.h
        Log/Log.cpp
        Log/Log.h
        Utils/mutex.h
        Utils/singleton.h)
