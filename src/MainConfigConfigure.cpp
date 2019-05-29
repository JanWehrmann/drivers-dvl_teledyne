#include <dvl_teledyne/Driver.hpp>
#include <dvl_teledyne/Config.hpp>
#include <iostream>

using namespace dvl_teledyne;

void usage()
{
    std::cerr << "dvl_teledyne_config_configure serial:///path/to/device:baudrate" << std::endl;
}

int main(int argc, char const* argv[])
{
    if (argc != 2)
    {
        usage();
        return 1;
    }

    dvl_teledyne::Driver driver;
    driver.open(argv[1]);

    Config config;

    driver.applyConfig(config);
}


