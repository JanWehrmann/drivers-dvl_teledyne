#ifndef DVL_TELEDYNE_DRIVER_HPP
#define DVL_TELEDYNE_DRIVER_HPP

#include <iodrivers_base/Driver.hpp>
#include <dvl_teledyne/PD0Parser.hpp>
#include <dvl_teledyne/Config.hpp>

namespace dvl_teledyne
{
    class Driver : public iodrivers_base::Driver, public PD0Parser
    {
        std::vector<uint8_t> buffer;
        int extractPacket (uint8_t const *buffer, size_t buffer_size) const;

        bool mConfMode;
        BAUDRATE mDesiredBaudrate;

    public:
        Driver();

        /** Tries to access the DVL at the provided URI
         *
         * For now, only a serial port can be provided. It is assumed that the
         * DVL is using 9600 bauds (the manufacturer's default)
         */
        void open(std::string const& uri);

        /** Once open using the baudrate specified in the URI, configures the
         * device to output at a different baud rate, and modifies the driver's
         * configuration accordingly
         */
        void setDesiredBaudrate(dvl_teledyne::BAUDRATE rate);

        /** Configures the output coordinate system */
        void setOutputConfiguration(OutputConfiguration conf);
        
        /** Sends a text file that contains commands to the device
         *
         * The device is guaranteed to be in configuration mode afterwards
         * (regardless of whether the configuration file contains a CS
         * command). Use startAcquisition() to put it in acquisition mode
         */
        void sendConfigurationFile(std::string const& file_name);

        /** Sets the device into configuration mode (and make it stop pinging)
         * */
        void setConfigurationMode();

        /** Start acquisition
         *
         * Since the driver relies on receiving PD0 message frames, this method
         * requires the DVL to send into this format, and then starts pinging
         */
        void startAcquisition();

        /** Read available packets on the I/O */
        void read();

        /** Verifies that the DVL acked a configuration command
         *
         * Throws std::runtime_error if an error is reported by the device
         */
        void readConfigurationAck(base::Time const& timeout = base::Time::fromSeconds(1));

        /** Configures the number of bottom-track pings per data ensemble.*/
        void setBottomTrackPingsPerEnsemble(int bottomTrackPingsPerEnsemble);

        /** Configures the maximum tracking depth (10 - 65535 dm).*/
        void setMaximumTrackingDepth(int maximumTrackingDepth);

        /** Configures the serial port control settings (stopBits : 1 or 2).*/
        void setSerialPortControlSettings(BAUDRATE baudrate, PARITY parity, int stopBits);

        /** Configures wether ensemble and ping cycling are handled
         * automatically or manually, wether data output is sent in binary or
         * hex-ASCII format, and wether data is sent via the serial interface or not.
         */
        void setFlowControlSettings(bool automaticEnsembleCycling,
                                    bool automaticPingCycling,
                                    bool binaryDataOutput,
                                    bool enableSerialOutput,
                                    bool enableDataRecording);

        /** Configures the correction between the heading reference and beam 3 of the
         * DVL (-17999 - 18000 hundreths of a degree).
         */
        void setHeadingAlignment(int headingAlignment);

        /** Same as setHeadingAlignment but corrects for electrical and magnetic bias
         * instead of physikal misalignment.*/
        void set_e_headingBias(int e_headingBias);

        /** Configures the waters salinity (0 - 40 parts per thousand).*/
        void setSalinity(int salinity);

        /** Configures the sensor sources (fixed manual value, internal sensor, external sensor).*/
        void setSensorSourceSettings(SENSORSOURCE speedOfSoundSource,
                                     SENSORSOURCE depthSource,
                                     SENSORSOURCE headingSource,
                                     SENSORSOURCE pitchAndRollSource,
                                     SENSORSOURCE salinitySource,
                                     SENSORSOURCE temperatureSource);

        /** Configures the minimum interval between ensemble acquisition (0 - 24h59'59.99").*/
        void setTimePerEnsemble(const base::Time& timePerEnsemble);

        /** Configures the minimum time between pings (0 - 59'59.99").*/
        void setTimeBetweenPings(const base::Time& timeBetweenPings);

        /** Configures the number of depth cells (1 - 255).*/
        void setNumberOfDepthCells(int numberOfDepthCells);

        /** Configures the number of pings per data ensemble during water profiling (0 - 16384).*/
        void setPingsPerEnsemble(int pingsPerEnsemble);

        /** Configures the height of one measurement cell (10 - 800 cm).*/
        void setDepthCellSize(int depthCellSize);

        /** Sends all configured settings to the dvl and saves them to non volatile memory.
         * NOTE: Sets baudrate, parity and stop bits last and tries saving again afterwards.
         * This will likely fail if you have changed those settings since you need to
         * reconfigure the sending device to match the new settings.
         */
        void applyConfig(const dvl_teledyne::Config& conf);

    protected:
        void sendStandardCommand(const std::string& characters, int value, int numDigits, bool sign = false, bool expert = false);
        int getNumberOfDigits(const std::string& stringOfValue) const;
        std::vector<int> splitTime(const base::Time& microseconds) const;
    };
}

#endif

