#include "hole.h"

#include "dmpacket.h"
#include "dmpacket2.h"
#include "helpers.h"
#include "vstdstr.h"

bool sift_up(const TDatagram& src, TDatagram2& dst)
{
  ConvertDtgrmStruct(src, dst);
  switch (src.Type) {
  case 0:
    dst.Size = 64;
    break;
  case 1:
    dst.Size = 120;
    break;
  default:
    break;
  }
  return true;
}

Hole::Hole(boost::asio::io_context& io_context)
    : timer_(io_context), ipx_(io_context), mcast_(io_context)
{
}

void Hole::handle_timeout(const boost::system::error_code& error)
{
  if (!error) {
    stat_work_and_clear();
    timer_.expires_after(std::chrono::milliseconds(dt_milliseconds_));
    timer_.async_wait([this](const boost::system::error_code& ec) { this->handle_timeout(ec); });
  }
}

bool Hole::start()
{
  std::string local_address=;
  if (!mcast_.open([](const std::string& e) { std::cerr << "MCAST: " << e << std::endl; }, local_address.c_str()))
    return false;

  bool nnov_nocodec=;
  if (!ipx_.open(!nnov_nocodec, [](const std::string& e) { std::cerr << "IPX: " << e << std::endl; }))
    return false;

  mcast_.start_receiver([this](const TDatagram2& d, const char* a) { push_down(d, a); });
  ipx_.start_receiver([this](const TDatagram& d) { push_up(d); });

  timer_.expires_after(std::chrono::milliseconds(dt_milliseconds_));
  timer_.async_wait([this](const boost::system::error_code& ec) { this->handle_timeout(ec); });

  return true;
}

void Hole::stat_work_and_clear()
{
  //печатаем
  std::string txt = str_printf( "ipx: %5d [%8d B/s]%s " //
                               , stat_up_.count_.load(), (unsigned)stat_up_.size_.load() * 1000 / dt_milliseconds_ //
                               );
  // clearing
  stat_up_.count_.exchange(0);
  stat_up_.size_.exchange(0);
  std::cout << Vx::now_time().c_str() << " " << txt << std::endl;
}

void Hole::push_up(const TDatagram& dtg)
{
  stat_up_.size_ += sizeof(dtg);
  TDatagram2 dtg2;
  if (sift_up(dtg, dtg2)) {
    mcast_.async_send(dtg2);
    stat_up_.count_++;
  }
}

void Hole::push_down(const TDatagram2&, const char*)
{
}
