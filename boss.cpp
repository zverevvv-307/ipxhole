#include "boss.h"
#include "helpers.h"
#include "opt.h"
#include "dmpacket2.h"
#include "dmpacket.h"
#include "vstdstr.h"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/filesystem.hpp>


static const std::string status_basesubname("ol2n");

Boss::Boss( boost::asio::io_context& io_context, TaskIpx& ipx, TaskMcast& mcast)
    :prn_timer_( io_context )
    ,dt_prn_milliseconds(10*1000)
    ,ipx_(ipx)
    ,mcast_(mcast)
    ,filter_()
    ,stat_ipx_()
    ,stat_mcast_()
    ,hrtbit_timer_( io_context )
    ,dt_hrtbit_milliseconds(350)
    ,my_uuid_( boost::uuids::random_generator()() )
    ,status_name_()
    ,level_(0)
    ,active_(false)
    ,max_level_(0)
    ,is_ipx_ok_(0)
{
}

void Boss::handle_timeout(const boost::system::error_code& error)
{
    if (!error) {
        stat_work_and_clear();
        prn_timer_.expires_from_now( boost::posix_time::milliseconds(dt_prn_milliseconds) );
        prn_timer_.async_wait( boost::bind(&Boss::handle_timeout, this, boost::asio::placeholders::error) );
    }
}

void Boss::handle_hrtbit_timeout(const boost::system::error_code& error)
{
    if (!error) {
        static int counter=0;
        counter++;

        send_status();
        if( counter%3 == 0 ){
            make_the_decision();
            send_status2();
        }

        hrtbit_timer_.expires_from_now( boost::posix_time::milliseconds(dt_hrtbit_milliseconds) );
        hrtbit_timer_.async_wait( boost::bind(&Boss::handle_hrtbit_timeout, this, boost::asio::placeholders::error) );
    }
}

bool Boss::start()
{
    if( !filter_.open( Options::instance()->config_file ) )
        return false;

    if( !mcast_.open([](const std::string& e){ std::cerr<<"MCAST: "<<e<<std::endl; }
                     ,Options::instance()->local_address.empty() ? nullptr : Options::instance()->local_address.c_str()
                     ) )
        return false;

    if( !ipx_.open( !Options::instance()->nnov_nocodec
                    , [](const std::string& e){ std::cerr<<"IPX: "<<e<<std::endl; } ) )
        return false;

    mcast_.start_receiver(
                [this](const TDatagram2& d, const char* a)
        { if(is_companion(d, a)==false) push_down(d, a); }
    );
    ipx_.start_receiver( [this](const TDatagram& d){push_up(d);} );


    prn_timer_.expires_from_now( boost::posix_time::milliseconds(dt_prn_milliseconds) );
    prn_timer_.async_wait( boost::bind(&Boss::handle_timeout, this, boost::asio::placeholders::error) );

    hrtbit_timer_.expires_from_now( boost::posix_time::milliseconds(dt_hrtbit_milliseconds) );
    hrtbit_timer_.async_wait( boost::bind(&Boss::handle_hrtbit_timeout, this, boost::asio::placeholders::error) );
    return true;
}

void Boss::stat_work_and_clear()
{
    //печатаем
    is_ipx_ok_ = stat_ipx_.size_.load();
    std::string txt = str_printf(
            "{%s} act=%s;  "
            "\t ipx: %5d [%8d B/s]%s "
            "\t mcast: %5d [%8d B/s] "
            , status_name_.c_str(), active_? "YES":"No"
            , stat_ipx_.count_.load(), (unsigned)stat_ipx_.size_.load()*1000/dt_prn_milliseconds, is_ipx_ok_?"":"-no connect?"
            , stat_mcast_.count_.load(), (unsigned)stat_mcast_.size_.load()*1000/dt_prn_milliseconds
            );
    //clearing
    stat_ipx_.count_.exchange(0);
    stat_ipx_.size_.exchange(0);
    stat_mcast_.count_.exchange(0);
    stat_mcast_.size_.exchange(0);
    std::cout << Vx::now_time().c_str() << " " << txt << std::endl;
}

void Boss::push_up(const TDatagram &dtg)
{
    stat_ipx_.size_+= sizeof(dtg);
    TDatagram2 dtg2;
    if( filter_.sift_up(dtg, dtg2) && active_ ) {
        mcast_.async_send(dtg2);
        stat_ipx_.count_++;
    }
}


#include "tTU.h"
void TranslateMyRTU(const TDatagram2 &tu, uint32_t status, TDatagram2* rtudtg);

void Boss::push_down(const TDatagram2 &dtg2, const char *a)
{
    stat_mcast_.size_+= dtg2.Size + TDatagram2_HEADER_LN;

    //syb: TU by IP -----------------------------------
    if(dtg2.Type==0 && active_){
        bool accept = filter_.check_host(a);
        TDatagram2 rtudtg = dtg2;
        rtudtg.Type = 2;
        TranslateMyRTU(dtg2, accept ? 98 : 97, &rtudtg);
        rtudtg.Size = sizeof(tRTU2);
        mcast_.async_send(rtudtg);
        if( !accept ) {
            std::cout << Vx::now_time().c_str() << "TU rejected from " << a << std::endl;
            return;
        }
    }

    TDatagram dtg;
    if( filter_.sift_down(dtg2, dtg) && active_ ) {
        ipx_.send(dtg);
        stat_mcast_.count_++;
    }
}

#include <random>
static std::random_device rd;
static std::mt19937 mt( rd() );

std::string Boss::generate_statusname(std::string& name, int& level)
{
    std::uniform_real_distribution<double> dist(1.0, 1000.0);
    level = dist(mt);
    if( Options::instance()->xp ) {
       size_t hash = hash_value(my_uuid_);
       std::cout << "uuid: " << to_string(my_uuid_)<<" hash=" << hash << std::endl;
       level = (level+hash)%1000;
    }
    name = str_printf( "%d_%s", level, status_basesubname.c_str());
    std::cout << std::endl << "generated statusname: " << name << std::endl;
    return name;
}

//выуживаем статусы с другого гейта сообщника
static TDatagram2 dtg2;//my status
bool Boss::is_companion(const TDatagram2& d, const char* addr)
{
    if( d.Type==3
            && std::string( d.Name, sizeof(d.Name) ).find(status_basesubname) != std::string::npos
            )
    {
        if( memcmp( my_uuid_.data, d.Data, my_uuid_.size() ) != 0 ) //от самогосебя пропускаем
        {//наш пациент с соседней кровати
            int lev2 = to_int(d.Name);
            if( level_ == lev2 ){
                level_=0; //выставим перегенерить свой уровень
                active_=false;
            }
            else{
                if( level_ < lev2 )// если появился старший
                    active_=false;
                max_level_ = std::max(max_level_, lev2);
            }
        }
        return true;
    }
    return false;
}

void Boss::send_status()
{
    if(level_==0){
        generate_statusname(status_name_, level_);
        dtg2.Type = 3;
        dtg2.setName( status_name_.c_str() );
        dtg2.setData(my_uuid_.data, my_uuid_.size());
        dtg2.Size = my_uuid_.size();
    }

    if(is_ipx_ok_==0 && active_)//если потеряли коннект ипикс, даем другим поработать, не шлем свой статус
        return;

    mcast_.async_send(dtg2);
}

//принимаем решение включать трансляцию ли
void Boss::make_the_decision(void)
{
    if( level_ > max_level_ )// если мы старший, начинаем работать
        active_=true;
    max_level_ = 0;
}

void Boss::send_status2()//syb status
{
#pragma pack(push, 1)
    struct Stat2{
        uint32_t activ;
        uint32_t file_size;
        uint32_t last_write_time;
    };
#pragma pack(pop)

    static TDatagram2 dtg;
    Stat2& stat2 = *(reinterpret_cast<Stat2*>(&dtg.Data[0]));

    if(!dtg.Type){//
        if(!dtg2.Type) return;

        dtg.Type = 3;
        std::string name(status_name_);
        std::reverse(name.begin(), name.end());
        dtg.setName( name.c_str() );
        dtg.Size = sizeof(stat2);

        using namespace boost::filesystem;
        path file_path(filter_.dbfile());
        stat2.file_size = file_size(file_path);
        stat2.last_write_time = last_write_time(file_path);
    }

    //вставляем
    stat2.activ = (active_ && is_ipx_ok_) ? 1 : 0;
    mcast_.async_send(dtg);
}



// xnya ........
static uint32_t decode(const tTU*  tu)
{
    uint32_t ctu;
    uint8_t KODA[4];
    KODA[0]=tu->tu[0].cod    ;
    KODA[1]=tu->tu[0].group  ;
    KODA[3]=tu->tu[1].group  ;
    KODA[2]=tu->tu[1].cod    ;
    memcpy(&ctu,&KODA,sizeof(ctu));
    return ctu;
}

void TranslateMyRTU(const TDatagram2 &tu, uint32_t status, TDatagram2* rtudtg)
{
    *rtudtg = tu;
    rtudtg->Type = 2;
    const tTU2*  tu32 = reinterpret_cast<const tTU2 *> (&tu.Data[0]);
    tRTU2* rtu = reinterpret_cast<tRTU2*>(&rtudtg->Data[0]);
    rtu->stan = tu32->TU.tu[0].stan;
    rtu->status = status;
    rtu->cod = decode( &tu32->TU );
    rtu->time = tu32->Time;
    rtu->id = tu32->Id;
    rtudtg->Size = sizeof(tRTU2);
}
