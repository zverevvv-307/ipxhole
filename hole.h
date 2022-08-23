#pragma once

#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>

#include "../gtlan/gteth.h"
#include "gtasioipx.h"

typedef GtAsioIpxSocket TaskIpx;
typedef GtEthernet      TaskMcast;

class Hole
{
    void handle_timeout(const boost::system::error_code& error);
    boost::asio::steady_timer timer_;
    int dt_milliseconds_{3000};

    TaskIpx ipx_;
    TaskMcast mcast_;

    struct Statistics{
        boost::atomic_int count_;
        boost::atomic_int size_;
    };
    Statistics stat_up_{};

public:

    Hole(boost::asio::io_context& io_context);
    bool start();

    void stat_work_and_clear();

    void push_up(const TDatagram &dtg);
    void push_down(const TDatagram2 &dtg2, const char* a);
};


