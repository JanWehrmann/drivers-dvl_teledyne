#ifndef DVL_TELEDYNE_DRIVER_HPP
#define DVL_TELEDYNE_DRIVER_HPP

#include <iodrivers_base/Driver.hpp>
#include <dvl_teledyne/PD0Parser.hpp>
#include <dvl_teledyne/Config.hpp>


namespace dvl_teledyne
{
    /**Driver implementation for the dvl_teledyne.
     *
     * Usage:
     * <ul>
     *  <li> Use iodrivers_base::Driver::openURI to open the connection to the device
     *  <li> Enter configuration Mode with setConfigurationMode.
     *  <li> Configure the driver if/as needed using sendConfigurationFile or applyConfig
     *       or the required set[Setting] methods.
     *  <li> Start data collection using startAcquisition.
     *  <li> Stop pinging with setConfigurationMode if needed.
     * </ul>
     */
    class Driver : public iodrivers_base::Driver, public PD0Parser
    {
        std::vector<uint8_t> buffer;
        int extractPacket (uint8_t const *buffer, size_t buffer_size) const;

        bool mConfMode;

    public:
        Driver();

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

        /** Configures the maximum tracking depth.
         * @param maximumTrackingDepth tracking depth in meters. Must be between 1 and 6553.5.
         */
        void setMaximumTrackingDepth(float maximumTrackingDepth);

        /** Configures the serial port control settings
         * @param baudrate the baudrate the dvl should use.
         * @param parity the parity the dvl should use.
         * @param stopBits must be either 1 or 2.
         * WARNING: Using this will almost certainly cause you to loose the connection to the
         *          device and require you to reconnect with the settings you just set using
         *          this method!
         */
        void setSerialPortControlSettings(BAUDRATE baudrate, PARITY parity, int stopBits);

        /** Configures how data acquisition and propagation are handled.
         *  NOTE: Consider the DVL user manual some additional information.
         * @param automaticEnsembleCycling:
         *      ENABLED:  Automatically starts the next data collection cycle after the
         *                current cycle is completed. Only a <BREAK> can stop this cycling.
         *      DISABLED: Enters the STANDBY mode after transmission of the data
         *                ensemble, displays the > prompt and waits for a new command.
         * @param automaticPingCycling:
         *      ENABLED:  Pings immediately when ready.
         *      DISABLED: Sends a > character to signal ready to ping, and then waits to
         *                receive an <Enter> before pinging. The <Enter> sent to the ExplorerDVL
         *                is not echoed.This feature lets you manually control ping timing within
         *                the ensemble.
         * @param binaryDataOutput:
         *      ENABLED:  Sends the ensemble in binary format, if serial output is
         *                enabled.
         *      DISABLED: Sends the ensemble in readable hexadecimal-ASCII format, if
         *                serial output is enabled.
         *                NOTE: Inteded for use when directly displaying data, i.e.
         *                in a terminal.
         * @param enableSerialOutput:
         *      ENABLED:  Sends the data ensemble out the serial interface.
         *      DISABLED: No data are sent out the serial interface.
         * @param enableDataRecording:
         *      ENABLED:  Sends data to the recorder if one is installed.
         *      DISABLED: Data is not being recorded.
         */
        void setFlowControlSettings(bool automaticEnsembleCycling,
                                    bool automaticPingCycling,
                                    bool binaryDataOutput,
                                    bool enableSerialOutput,
                                    bool enableDataRecording);

        /** Configures the correction between the heading reference and beam 3 of the
         * DVL.
         * @param headingAlignment offset in radians, between -179.99 and +180 degrees.
         */
        void setHeadingAlignment(base::Angle headingAlignment);

        /** Same as setHeadingAlignment() but corrects for electrical and magnetic bias
         * instead of physical misalignment.
         * @param e_headingBias offset in radians, between -179.99 and +180 degrees.
         */
        void set_e_headingBias(base::Angle e_headingBias);

        /** Configures the waters salinity.
         * @param salinity in parts per thousand. Must be between 0 and 40.
         */
        void setSalinity(int salinity);

        /** Configures the sensor sources (fixed manual value, internal sensor, external sensor).*/
        void setSensorSourceSettings(SENSORSOURCE speedOfSoundSource,
                                     SENSORSOURCE depthSource,
                                     SENSORSOURCE headingSource,
                                     SENSORSOURCE pitchAndRollSource,
                                     SENSORSOURCE salinitySource,
                                     SENSORSOURCE temperatureSource);

        /** Configures the minimum interval between ensemble acquisition.
         * @param timePerEnsemble must be between 0 and 24h59'59.99".
         */
        void setTimePerEnsemble(const base::Time& timePerEnsemble);

        /** Configures the minimum time between pings.
         * @param timeBetweenPings must be between 0 and 59'59.99".
         */
        void setTimeBetweenPings(const base::Time& timeBetweenPings);

        /** Configures the number of depth cells.
         * @param numberOfDepthCells must be between 1 and 255.
         */
        void setNumberOfDepthCells(int numberOfDepthCells);

        /** Configures the number of pings per data ensemble during water profiling.
         * @param pingsPerEnsemble must be between 0 and 16384.
         */
        void setPingsPerEnsemble(int pingsPerEnsemble);

        /** Configures the height of one measurement cell.
         * @param depthCellSize the size of a depth cell in meters. Must be between 0.01 and 8.
         */
        void setDepthCellSize(float depthCellSize);

        /** Sends all configured settings to the dvl and saves them to non volatile memory.
         */
        void applyConfig(const dvl_teledyne::Config& conf);

    protected:
        /** Sends a standard formatted command to the dvl.
         * @param characters Characters that specify the command, e.g.: BP
         * @param value Value the corresponding setting shall be set to
         * @param numDigits number of digits the value has in the expected format of the command, e.g.: BP001 -> numDigits = 3
         *                  This will be used to fill the command string with the needed amount of zeros to fit the format.
         * @param sign set true if the command format includes a sign, e.g.: EA+04500
         */
        void sendStandardCommand(const std::string& characters, int value, int numDigits, bool sign = false);

        /** Internal method used in sendStandardCommand(). Allows for easier testing.*/
        std::string parseStandardCommand(const std::string& characters, int value, int numDigits, bool sign = false);
    };
}

#endif

