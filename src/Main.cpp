#include <dvl_teledyne/Driver.hpp>
#include <iostream>
#include <signal.h>

using namespace dvl_teledyne;

volatile sig_atomic_t quit = 0;

void handleSignal(int signal)
{
    quit = 1;
}

void usage()
{
    std::cerr << "dvl_teledyne_read serial://PATH/TO/DEVICE:BAUDRATE" << std::endl;
}

int main(int argc, char const* argv[])
{
    if (argc != 2)
    {
        usage();
        return 1;
    }
    signal(SIGINT, handleSignal);

    dvl_teledyne::Driver driver;
    driver.openURI(argv[1]);
    driver.startAcquisition();
    driver.setReadTimeout(base::Time::fromSeconds(5));
    driver.read();

    char const* coord_systems[4] = { "BEAM", "INSTRUMENT", "SHIP", "EARTH" };
    std::cout << "Device outputs its data in the " << coord_systems[driver.outputConf.coordinate_system] << " coordinate system" << std::endl;


    std::cout << "Time Seq ";
    for (int beam = 0; beam < 4; ++beam)
        std::cout << " range[" << beam << "] velocity[" << beam << "] evaluation[" << beam << "]";
    std::cout << std::endl;

    std::cout << std::endl << "Press CTRL + C to stop aquisition and exit the program." << std:: endl << std::endl;

    while(!quit)
    {
        driver.read();

        BottomTracking const& tracking = driver.bottomTracking;
        std::cout << tracking.time.toString() << " " << driver.status.seq;
        for (int beam = 0; beam < 4; ++beam)
            std::cout << " " << tracking.range[beam] << " " << tracking.velocity[beam] << " " << tracking.evaluation[beam];
        std::cout << std::endl;
    }
    std::cout << std::endl << "Stopping data aquisition and shutting down." << std::endl;
    driver.setConfigurationMode();
}


