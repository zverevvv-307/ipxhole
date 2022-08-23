#include <iostream>
#include <iomanip>

#include "hole.h"
#include "asio_pch.h"


using namespace boost::asio;

int main(int argc, char *argv[])
{
#ifdef GIT_REVISION
    std::cout << "Version: " << GIT_REVISION << std::endl;
#endif
    try {
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        asio_lib::Executor e;
        e.OnWorkerThreadError = [](io_context&, boost::system::error_code ec) {
            std::cerr<<"\n!!! error (asio): "<<ec.message();
        };
        e.OnWorkerThreadException = [](io_context&, const std::exception &ex) {
            std::cerr<<"\n!!! exception (asio): "<<ex.what();
        };
        e.AddCtrlCHandling();

        Hole hole( e.GetIOContext() );
        if ( hole.start() )
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
