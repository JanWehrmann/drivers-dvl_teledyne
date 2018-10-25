#include <dvl_teledyne/Driver.hpp>
#include <sys/ioctl.h>
#include <termios.h>
#include <iostream>
#include <fstream>

using namespace dvl_teledyne;

Driver::Driver()
    : iodrivers_base::Driver(1000000)
    , mConfMode(false)
    , mDesiredBaudrate(BR9600)
{
    buffer.resize(1000000);
}

void Driver::open(std::string const& uri)
{
    openURI(uri);
    setConfigurationMode();
    if (mDesiredBaudrate != BR9600)
        setDesiredBaudrate(mDesiredBaudrate);

    startAcquisition();
}

void Driver::sendConfigurationFile(std::string const& file_name)
{
    setConfigurationMode();

    std::ifstream file(file_name.c_str());

    char line_buffer[2000];
    while (!file.eof())
    {
        if (!file.getline(line_buffer, 2000) && !file.eof())
            throw std::runtime_error("lines longer than 2000 characters");

        std::string line(line_buffer);
        if (line == "CS")
            break;

        line += "\n";
        std::cout << iodrivers_base::Driver::printable_com(line) << std::endl;
        writePacket(reinterpret_cast<uint8_t const*>(line.c_str()), line.length());
        readConfigurationAck();
    }
}

void Driver::setDesiredBaudrate(BAUDRATE rate)
{
    if (getFileDescriptor() != iodrivers_base::Driver::INVALID_FD)
        setSerialPortControlSettings(rate, NONE, 1);
    mDesiredBaudrate = rate;
}

void Driver::read()
{
    int packet_size = readPacket(&buffer[0], buffer.size());
    if (packet_size)
        parseEnsemble(&buffer[0], packet_size);
}

int Driver::extractPacket (uint8_t const *buffer, size_t buffer_size) const
{
    if (mConfMode)
    {
        char const* buffer_as_string = reinterpret_cast<char const*>(buffer);
        if (buffer_as_string[0] == '>')
            return 1;
        else if (buffer_as_string[0] == 'E')
        {
            if (buffer_size > 1 && buffer_as_string[1] != 'R')
                return -1;
            else if (buffer_size > 2 && buffer_as_string[2] != 'R')
                return -1;

            // We have an error. Find \n> and return
            size_t eol = 2;
            for (eol = 2; eol < buffer_size - 1; ++eol)
            {
                if (buffer_as_string[eol] == '\n' && buffer_as_string[eol + 1] == '>')
                    return eol;
            }
            return 0;
        }
        else
            return -1;
    }
    else
    {
        // std::cout << iodrivers_base::Driver::printable_com(buffer, buffer_size) << std::endl;
        return PD0Parser::extractPacket(buffer, buffer_size);
    }
}

void Driver::setConfigurationMode()
{
    if (mConfMode)
        return;

    if (tcsendbreak(getFileDescriptor(), 0))
        throw iodrivers_base::UnixError("failed to set configuration mode");
    mConfMode = true;

    // This is a tricky one. As usual with fiddling with serial lines, the
    // device is inaccessible "for a while" (which is unspecified)
    //
    // Repeatedly write a CR on the line and check for an ack (i.e. a prompt).
    // We do it repeatedly so that we are sure that the CR is not lost.
    clear();
    for (int i = 0; i < 12; ++i)
    {
        writePacket(reinterpret_cast<uint8_t const*>("\n"), 1, 100);
        try
        {
            readConfigurationAck(base::Time::fromSeconds(0.1));
            clear();
            break;
        }
        catch(iodrivers_base::TimeoutError)
        {
            if (i == 11) throw;
        }
    }
}

void Driver::readConfigurationAck(base::Time const& timeout)
{
    if (!mConfMode)
        throw std::runtime_error("not in configuration mode");
    int packet_size = readPacket(&buffer[0], buffer.size(), timeout);
    if (buffer[0] != '>')
        throw std::runtime_error(std::string(reinterpret_cast<char const*>(&buffer[0]), packet_size));
}

/** Configures the output coordinate system */
void Driver::setOutputConfiguration(OutputConfiguration conf)
{
    if (!mConfMode)
        throw std::runtime_error("not in configuration mode");

    uint8_t mode_codes_1[4] = { '0', '0', '1', '1' };
    uint8_t mode_codes_2[4] = { '0', '1', '0', '1' };
    uint8_t const cmd[7] = {
        'E', 'X',
        mode_codes_1[conf.coordinate_system], mode_codes_2[conf.coordinate_system],
        (conf.use_attitude       ? '1' : '0'),
        (conf.use_3beam_solution ? '1' : '0'),
        (conf.use_bin_mapping ? '1' : '0') };

    writePacket(cmd, 7, 500);
    readConfigurationAck();
}

void Driver::startAcquisition()
{
    if (!mConfMode)
        throw std::logic_error("not in configuration mode");

    writePacket(reinterpret_cast<uint8_t const*>("PD0\n"), 4, 100);
    readConfigurationAck();
    writePacket(reinterpret_cast<uint8_t const*>("CS\n"), 3, 100);
    readConfigurationAck();
    mConfMode = false;
}

void Driver::setBottomTrackPingsPerEnsemble(int bottomTrackPingsPerEnsemble)
{
    sendStandardCommand(std::string("BP"), bottomTrackPingsPerEnsemble, 3);
}

void Driver::setMaximumTrackingDepth(int maximumTrackingDepth)
{
    sendStandardCommand(std::string("BX"), maximumTrackingDepth, 5);
}

void Driver::setSerialPortControlSettings(BAUDRATE baudrate, PARITY parity, int stopBits)
{
    setConfigurationMode();
    writePacket(reinterpret_cast<uint8_t const*>(("CB" + std::to_string(baudrate)
                                                      + std::to_string(parity)
                                                      + std::to_string(stopBits)
                                                      + "\n").c_str()), 6);
    readConfigurationAck();
}

void Driver::setFlowControlSettings(bool automaticEnsembleCycling,
                                    bool automaticPingCycling,
                                    bool binaryDataOutput,
                                    bool enableSerialOutput,
                                    bool enableDataRecording)
{
    setConfigurationMode();
    writePacket(reinterpret_cast<uint8_t const*>(("CF" + std::to_string(automaticEnsembleCycling)
                                                      + std::to_string(automaticPingCycling)
                                                      + std::to_string(binaryDataOutput)
                                                      + std::to_string(enableSerialOutput)
                                                      + std::to_string(enableDataRecording)
                                                      + "\n").c_str()), 8);
    readConfigurationAck();
}

void Driver::setHeadingAlignment(int headingAlignment)
{
    sendStandardCommand(std::string("EA"), headingAlignment, 5, true);
}

void Driver::set_e_headingBias(int e_headingBias)
{
    sendStandardCommand(std::string("#EV"), e_headingBias, 5, true, true);
}

void Driver::setSalinity(int salinity)
{
    sendStandardCommand(std::string("ES"), salinity, 2);
}

void Driver::setSensorSourceSettings(SENSORSOURCE speedOfSoundSource,
                                     SENSORSOURCE depthSource,
                                     SENSORSOURCE headingSource,
                                     SENSORSOURCE pitchAndRollSource,
                                     SENSORSOURCE salinitySource,
                                     SENSORSOURCE temperatureSource)
{
    setConfigurationMode();
    writePacket(reinterpret_cast<uint8_t const*>(("EZ" + std::to_string(speedOfSoundSource)
                                                      + std::to_string(depthSource)
                                                      + std::to_string(headingSource)
                                                      + std::to_string(pitchAndRollSource)
                                                      + std::to_string(salinitySource)
                                                      + std::to_string(temperatureSource)
                                                      + "\n").c_str()), 7);
    readConfigurationAck();
}

void Driver::setTimePerEnsemble(const base::Time& timePerEnsemble)
{
    setConfigurationMode();
    std::vector<int> timeSplits = splitTime(timePerEnsemble);
    std::vector<std::string> timeStrings;

    for (int i = 0; i < 4; ++i)
    {
        std::string timeString = std::to_string(timeSplits.at(i));
        if (getNumberOfDigits(timeString) == 1)
            timeString = "0" + timeString;
        timeStrings.push_back(timeString);
    }

    writePacket(reinterpret_cast<uint8_t const*>(("TE" + timeStrings.at(3) + ":"
                                                      + timeStrings.at(2) + ":"
                                                      + timeStrings.at(1) + "."
                                                      + timeStrings.at(0) + "\n").c_str()), 14);
    readConfigurationAck();
}

void Driver::setTimeBetweenPings(const base::Time& timeBetweenPings)
{
    setConfigurationMode();
    std::vector<int> timeSplits = splitTime(timeBetweenPings);
    std::vector<std::string> timeStrings;

    for (int i = 0; i < 4; ++i)
    {
        std::string timeString = std::to_string(timeSplits.at(i));
        if (getNumberOfDigits(timeString) == 1)
            timeString = "0" + timeString;
        timeStrings.push_back(timeString);
    }

    writePacket(reinterpret_cast<uint8_t const*>(("TP" + timeStrings.at(3) + ":"
                                                      + timeStrings.at(2) + ":"
                                                      + timeStrings.at(1) + "."
                                                      + timeStrings.at(0) + "\n").c_str()), 14);
    readConfigurationAck();
}

void Driver::setNumberOfDepthCells(int numberOfDepthCells)
{
    sendStandardCommand(std::string("WN"), numberOfDepthCells, 3);
}

void Driver::setPingsPerEnsemble(int pingsPerEnsemble)
{
    sendStandardCommand(std::string("WP"), pingsPerEnsemble, 5);
}

void Driver::setDepthCellSize(int depthCellSize)
{
    sendStandardCommand(std::string("WS"), depthCellSize, 4);
}

void Driver::applyConfig(const dvl_teledyne::Config& conf)
{
    setBottomTrackPingsPerEnsemble(conf.bottomTrackPingsPerEnsemble);
    setMaximumTrackingDepth(conf.maximumTrackingDepth);
    setFlowControlSettings(conf.automaticEnsembleCycling,
                                conf.automaticPingCycling,
                                conf.binaryDataOutput,
                                conf.enableSerialOutput,
                                conf.enableDataRecording);
    setHeadingAlignment(conf.headingAlignment);
    setSalinity(conf.salinity);
    set_e_headingBias(conf.e_headingBias);

    OutputConfiguration outputConf;
    outputConf.coordinate_system = conf.transformation;
    outputConf.use_attitude = conf.useTiltsInTransformation;
    outputConf.use_3beam_solution = conf.allow3BeamSolutions;
    outputConf.use_bin_mapping = conf.allowBinMapping;
    setOutputConfiguration(outputConf);

    setSensorSourceSettings(conf.speedOfSoundSource,
                                    conf.depthSource,
                                    conf.headingSource,
                                    conf.pitchAndRollSource,
                                    conf.salinitySource,
                                    conf.temperatureSource);
    setTimePerEnsemble(conf.timePerEnsemble);
    setTimeBetweenPings(conf.timeBetweenPings);
    setNumberOfDepthCells(conf.numberOfDepthCells);
    setPingsPerEnsemble(conf.pingsPerEnsemble);
    setDepthCellSize(conf.depthCellSize);

    // Save in case communication fails after altering serial port control settings.
    writePacket(reinterpret_cast<uint8_t const*>("CK\n"), 3, 100);
    readConfigurationAck();

    setSerialPortControlSettings(conf.baudrate, conf.parity, conf.stopBits);
    // Save again to save serialPortControlSettings.
    writePacket(reinterpret_cast<uint8_t const*>("CK\n"), 3, 100);
    readConfigurationAck();
}

void Driver::sendStandardCommand(const std::string& characters, int value, int numDigits, bool sign, bool expert)
{
    setConfigurationMode();
    std::string command = characters;

    if (sign && value < 0)
        command += "-";
    else if (sign)
        command += "+";

    for (int n = getNumberOfDigits(value); n < numDigits; ++n)
    {
        command += "0";
    }

    command += std::to_string(value);

    if (sign)
        ++numDigits;
    if (expert)
        ++numDigits;

    writePacket(reinterpret_cast<uint8_t const*>((command + "\n").c_str()), numDigits + 3);
    readConfigurationAck();
}

int Driver::getNumberOfDigits(const std::string& stringOfValue) const
{
    int numDigits = stringOfValue.size() - 1;

    return numDigits;
}

std::vector<int> Driver::splitTime(const base::Time& microseconds) const
{
    int centiseconds = microseconds.microseconds / 10000;
    int hours = centiseconds / 360000;
    centiseconds -= hours * 360000;
    int minutes = centiseconds / 6000;
    centiseconds -= minutes * 6000;
    int seconds = centiseconds / 100;
    centiseconds -= seconds * 100;

    std::vector<int> timeSplits;
    timeSplits.push_back(centiseconds);
    timeSplits.push_back(seconds);
    timeSplits.push_back(minutes);
    timeSplits.push_back(hours);

    return timeSplits;
}
