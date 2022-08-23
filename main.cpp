#include <iostream>
#include <iomanip>

#include "opt.h"
#include "lan_tasks.h"
#include "boss.h"
#include "asio_pch.h"


//using namespace std;
using namespace boost::asio;

int main(int argc, char *argv[])
{
#ifdef GIT_REVISION
    std::cout << "Version: " << GIT_REVISION << std::endl;
#endif
    try {
        if( Options::instance()->parse_args(argc, argv) <= 0 )
            return 0;
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        asio_lib::Executor e;
        e.OnWorkerThreadError = [](io_context&, boost::system::error_code ec) {
            std::cerr<<"\n!!! error (asio): "<<ec.message();
        };
        e.OnWorkerThreadException = [](io_context&, const std::exception &ex) {
            std::cerr<<"\n!!! exception (asio): "<<ex.what();
        };
        e.AddCtrlCHandling();

        TaskIpx task0(e.GetIOContext());
        TaskMcast  task1(e.GetIOContext());
        Boss boss(e.GetIOContext(), task0, task1);
        if ( boss.start() )
            e.Run(3);
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    }
    catch (std::exception& e) {
        std::cerr<<"\nmain->std::exception: "<<e.what()<<"\n";
    }
    catch (...) {
        std::cerr<<"\nmain->catch (...) ...\n";
    }

    return 0;
}
//------------------------------------------------------------------------------
