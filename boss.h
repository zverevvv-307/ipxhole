//----------------------------------------------------------------------------
//  Copyright 2017. All Rights Reserved.
//  Author:   Vladislav Zverev
//----------------------------------------------------------------------------
#ifndef BOSS_H
#define BOSS_H

#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/atomic.hpp>
#include <boost/uuid/uuid.hpp>
#include "lan_tasks.h"
#include "gtfilter.h"


class Boss
{
    void handle_timeout(const boost::system::error_code& error);
    boost::asio::deadline_timer prn_timer_;
    int dt_prn_milliseconds;

    TaskIpx& ipx_;
    TaskMcast& mcast_;
    GtFilter filter_;

    struct Statistics{
        boost::atomic_int count_;
        boost::atomic_int size_;
    };
    Statistics stat_ipx_;
    Statistics stat_mcast_;

    void handle_hrtbit_timeout(const boost::system::error_code& error);
    boost::asio::deadline_timer hrtbit_timer_;
    int dt_hrtbit_milliseconds;

    boost::uuids::uuid my_uuid_;//для фильтрации самого себя
    std::string  status_name_;
    mutable int level_; //наш уровень
    mutable bool active_;//работаем ли мы
    mutable int max_level_; //max уровень сообщников
    mutable bool is_ipx_ok_; //есть ли что на конце ипикс
    bool is_companion(const TDatagram2& d, const char* addr);
    void send_status();
    void send_status2();
    void make_the_decision(void);
    std::string generate_statusname(std::string& name, int& level);


public:

    Boss(boost::asio::io_context& io_context, TaskIpx& ipx, TaskMcast& mcast);
    bool start();

    void stat_work_and_clear();

    void push_up(const TDatagram &dtg);
    void push_down(const TDatagram2 &dtg2, const char* a);
};


#endif // BOSS_H
