#include "gtfilter.h"

#include<iostream>
#include<map>
#include <boost/filesystem.hpp>
#include "sqlite_modern_cpp.h"
#include "vstdstr.h"

using namespace sqlite;
using namespace std;
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
GtFilter::GtFilter()
= default;
//---------------------------------------------------------------------------
#include <unistd.h>
inline bool exists_file (const std::string& name) {
    return ( access( name.c_str(), F_OK ) != -1 );
}
bool GtFilter::open(const std::string& fn)
{
    filter[0].clear();
    filter[1].clear();
    dc_hosts.clear();
    if( !exists_file(fn) ){
        cerr << "!!! no file: " << fn << endl;
        return false;
    }
    dbname = fn;

    try{
        database db(fn);
        int count = 0;
        db << "select count(*) from sqlite_master where name='dtgdef';" >> tie(count);
        if (count == 0) {
            cerr<<"bad table: dtgdef."<<endl;
            return false;
        }
        db << "select type, subname, direction, sz from dtgdef ;"
           >> [&](int typ, string nm, int d, int sz) {
            FilterDataDef def;
            def.subname = trim(nm);
            def.sz = sz;
            if (d==1)
                filter[0].emplace( typ, def );
            if (d==2)
                filter[1].emplace( typ, def );
        };

        db << "select count(*) from sqlite_master where name='dchosts';" >> tie(count);
        if (count) {
            db << "select addr from dchosts ;"
                >> [&](string addr) {
                      dc_hosts.emplace( addr );
                  };
        }
        else
            cerr<<"bad table: dchosts."<<endl;

        cout << endl <<"Filters:"<< endl;
        print();
        cout << endl;
        return true;
    }
    catch(const sqlite_exception& e) {
        cerr <<endl<<__FUNCTION__<< " sqlite: " << e.what() << endl;
    }
    return false;
}

void GtFilter::print()
{
    cout << "filter-up: " << endl;
    for(const auto& i : filter[0])
        cout << i.first << ' ' << i.second.subname << ' ' << i.second.sz << endl;
    cout << "filter-down: " << endl;
    for(const auto& i : filter[1])
        cout << i.first << ' ' << i.second.subname << ' ' << i.second.sz << endl;
    cout << "filter-hosts: " << endl;
    for(const auto& i : dc_hosts)
        cout << i << endl;
}

#include "tTU.h"
static const uint8_t MARK=255;
bool GtFilter::sift_up(const TDatagram& src, TDatagram2& dst)
{
    //    std::cout << "sift "<<int(src.Type)<<":"<<src.Name<<endl;
    const FilterDataDef* p = check(filter[0], src.Type, string(src.Name) );
    if ( p != nullptr )  {
        ConvertDtgrmStruct(src, dst);
        dst.Size=p->sz;

        if (dst.Type==0){
            //2021 для энергов для ТУ архиватора чтоб не петля
            auto tu = reinterpret_cast<tTU*>(dst.Data);
            if(tu->tu[1].mid==MARK)
                return false;
            else
                tu->tu[1].mid=MARK;
        }

        return true;
    }
    return false;
}

bool GtFilter::sift_down(const TDatagram2& src, TDatagram& dst)
{
    //    std::cout << "sift "<<int(src.Type)<<":"<<src.Name<<endl;
    const FilterDataDef* p = check(filter[1], src.Type, string(src.Name) );
    if ( p != nullptr )  {
        ConvertDtgrmStruct(src, dst);

        if (dst.Type==0){
            //2021 для энергов для ТУ архиватора чтоб не петля
            auto tu = reinterpret_cast<tTU*>(dst.Data);
            if(tu->tu[1].mid==MARK)
                return false;
            else
                tu->tu[1].mid=MARK;
        }

        return true;
    }
    return false;
}

const GtFilter::FilterDataDef* GtFilter::check(const TFilterDefs& filter, int typ, const string& name )
{
    int count = filter.count(typ);
    if(count==1){
        auto f = filter.find(typ);
        if( f->second.subname=="*" || name.find(f->second.subname) != string::npos )
            return &f->second;
    }
    if(count>1){
        auto rng = filter.equal_range(typ);
        while(rng.first != rng.second){
            auto f = rng.first++;
            if( name.find(f->second.subname) != string::npos )
                return &f->second;
        }
    }
    return nullptr;
}

bool GtFilter::check_host(const string& addr)
{
    auto p = dc_hosts.find(addr);
    if ( p != dc_hosts.end() )  {
        return true;
    }
    return false;
}
