//----------------------------------------------------------------------------
//  Copyright 2017. All Rights Reserved.
//  Author:   Vladislav Zverev
//----------------------------------------------------------------------------
#ifndef GTFILTER_H
#define GTFILTER_H

#include <map>
#include <set>
#include "dmpacket2.h"
#include "dmpacket.h"


class GtFilter
{
    struct FilterDataDef {
        std::string subname;
        int         sz;
    };
    typedef std::multimap<int, FilterDataDef> TFilterDefs;
    TFilterDefs filter[2];

    typedef std::set<std::string > TAddrFilter;
    TAddrFilter dc_hosts;

    std::string dbname;

    const FilterDataDef* check(const TFilterDefs& filter, int typ, const std::string& name );

public:
    GtFilter();
    bool open(const std::string& fn);

    bool sift_up(const TDatagram& src, TDatagram2& dst);
    bool sift_down(const TDatagram2& src, TDatagram& dst);
    bool check_host(const std::string &addr);

    void print();
    const std::string& dbfile(){ return dbname; }
};

#endif // GTFILTER_H
