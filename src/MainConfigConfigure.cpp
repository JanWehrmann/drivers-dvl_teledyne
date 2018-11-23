#include <dvl_teledyne/Driver.hpp>
#include <dvl_teledyne/Config.hpp>
#include <iostream>

using namespace dvl_teledyne;

void usage()
{
    std::cerr << "dvl_teledyne_config_configure DEVICE" << std::endl;
}

int main(int argc, char const* argv[])
{
    if (argc != 2)
    {
        usage();
        return 1;
    }

    dvl_teledyne::Driver driver;
    driver.openSerial(argv[1], 9600);
    driver.setWriteTimeout(base::Time::fromSeconds(5));
    driver.setReadTimeout(base::Time::fromSeconds(5));

    Config config;

    driver.applyConfig(config);
}


