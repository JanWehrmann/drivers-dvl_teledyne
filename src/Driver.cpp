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
    m_read_timeout = base::Time::fromSeconds(1.);
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
        readConfigurationAck(m_read_timeout);
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
            readConfigurationAck(m_read_timeout);
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
    uint8_t const cmd[8] = {
        'E', 'X',
        mode_codes_1[conf.coordinate_system], mode_codes_2[conf.coordinate_system],
        (conf.use_attitude       ? '1' : '0'),
        (conf.use_3beam_solution ? '1' : '0'),
        (conf.use_bin_mapping ? '1' : '0'), '\n' };

    writePacket(cmd, 8, 500);
    readConfigurationAck(m_read_timeout);
}

void Driver::startAcquisition()
{
    if (!mConfMode)
        throw std::logic_error("not in configuration mode");

    writePacket(reinterpret_cast<uint8_t const*>("PD0\n"), 4, 100);
    readConfigurationAck(m_read_timeout);
    writePacket(reinterpret_cast<uint8_t const*>("CS\n"), 3, 100);
    mConfMode = false;
}

void Driver::setBottomTrackPingsPerEnsemble(int bottomTrackPingsPerEnsemble)
{
    sendStandardCommand(std::string("BP"), bottomTrackPingsPerEnsemble, 3);
}

void Driver::setMaximumTrackingDepth(float maximumTrackingDepth)
{
    // Convert from meter to decimeters.
    int maximumTrackingDepthInt = static_cast<int>(maximumTrackingDepth * 10);
    sendStandardCommand(std::string("BX"), maximumTrackingDepthInt, 5);
}

void Driver::setSerialPortControlSettings(BAUDRATE baudrate, PARITY parity, int stopBits)
{
    setConfigurationMode();
    writePacket(reinterpret_cast<uint8_t const*>(("CB" + std::to_string(baudrate)
                                                      + std::to_string(parity)
                                                      + std::to_string(stopBits)
                                                      + "\n").c_str()), 6);
    readConfigurationAck(m_read_timeout);
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
    readConfigurationAck(m_read_timeout);
}

void Driver::setHeadingAlignment(base::Angle headingAlignment)
{
    // Convert from radians to hundredths of a degree.
    int headingAlignmentInt = static_cast<int>(headingAlignment.getDeg() * 100);
    sendStandardCommand(std::string("EA"), headingAlignmentInt, 5, true);
}

void Driver::set_e_headingBias(base::Angle e_headingBias)
{
    // Convert from radians to hundredths of a degree.
    int headingBiasInt = static_cast<int>(e_headingBias.getDeg() * 100);
    sendStandardCommand(std::string("#EV"), headingBiasInt, 5, true);
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
                                                      + "0\n").c_str()), 10);
    readConfigurationAck(m_read_timeout);
}

void Driver::setTimePerEnsemble(const base::Time& timePerEnsemble)
{
    setConfigurationMode();
    std::vector<int> timeValues = timePerEnsemble.toTimeValues();
    std::vector<std::string> timeStrings;

    for (int i = 1; i < 5; ++i)
    {
        if (i == 1)
            timeValues.at(i) /= 10;
        std::string timeString = std::to_string(timeValues.at(i));
        if (timeString.size() == 1)
            timeString = "0" + timeString;
        timeStrings.push_back(timeString);
    }

    writePacket(reinterpret_cast<uint8_t const*>(("TE" + timeStrings.at(3) + ":"
                                                      + timeStrings.at(2) + ":"
                                                      + timeStrings.at(1) + "."
                                                      + timeStrings.at(0) + "\n").c_str()), 14);
    readConfigurationAck(m_read_timeout);
}

void Driver::setTimeBetweenPings(const base::Time& timeBetweenPings)
{
    setConfigurationMode();
    std::vector<int> timeValues = timeBetweenPings.toTimeValues();
    std::vector<std::string> timeStrings;

    for (int i = 1; i < 4; ++i)
    {
        if (i == 1)
            timeValues.at(i) /= 10;
        std::string timeString = std::to_string(timeValues.at(i));
        if (timeString.size() == 1)
            timeString = "0" + timeString;
        timeStrings.push_back(timeString);
    }

    writePacket(reinterpret_cast<uint8_t const*>(("TP" + timeStrings.at(2) + ":"
                                                       + timeStrings.at(1) + "."
                                                       + timeStrings.at(0) + "\n").c_str()), 11);
    readConfigurationAck(m_read_timeout);
}

void Driver::setNumberOfDepthCells(int numberOfDepthCells)
{
    sendStandardCommand(std::string("WN"), numberOfDepthCells, 3);
}

void Driver::setPingsPerEnsemble(int pingsPerEnsemble)
{
    sendStandardCommand(std::string("WP"), pingsPerEnsemble, 5);
}

void Driver::setDepthCellSize(float depthCellSize)
{
    // Convert from meters to centimeters.
    int depthCellSizeInt = static_cast<int>(depthCellSize * 100);
    sendStandardCommand(std::string("WS"), depthCellSizeInt, 4);
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
    readConfigurationAck(m_read_timeout);

    setSerialPortControlSettings(conf.baudrate, conf.parity, conf.stopBits);
    // Save again to save serialPortControlSettings.
    writePacket(reinterpret_cast<uint8_t const*>("CK\n"), 3, 100);
    readConfigurationAck(m_read_timeout);
}

void Driver::sendStandardCommand(const std::string& characters, int value, int numDigits, bool sign)
{
    setConfigurationMode();

    std::string command = parseStandardCommand(characters, value, numDigits, sign);

    writePacket(reinterpret_cast<uint8_t const*>((command).c_str()), command.size());
    readConfigurationAck(m_read_timeout);
}

std::string Driver::parseStandardCommand(const std::string& characters, int value, int numDigits, bool sign)
{
    std::string command = characters;

    if (sign && value < 0)
        command += "-";
    else if (sign)
        command += "+";

    std::string valueString = std::to_string(std::abs(value));

    // Here numDigits is used to fill in zeros until the command has the intended length.
    for (int n = valueString.size(); n < numDigits; ++n)
    {
        command += "0";
    }

    command = command + valueString + "\n";
    return command;
}

