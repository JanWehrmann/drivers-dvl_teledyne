#ifndef __DVL_TELEDYNE_CONFIG_HPP_
#define __DVL_TELEDYNE_CONFIG_HPP_

#include <base/Time.hpp>
#include <dvl_teledyne/PD0Messages.hpp>

namespace dvl_teledyne
{


enum BAUDRATE
{
    BR300,
    BR1200,
    BR2400,
    BR4800,
    BR9600,
    BR19200,
    BR38400,
    BR57600,
    BR115200
};

enum PARITY
{
    NONE = 1,
    EVEN,
    ODD,
    LOW,
    HIGH
};

enum SENSORSOURCE
{
    MANUAL,
    INTERNAL,
    EXTERNAL
};

/**DVL_teledyne configuration.
 * Holds selected settings to be sent to the DVL.
 * Initialized with factory defaults.
 * For a list of all configuration commands and detailed information of the
 * ones used here, please read the ExplorerDVL Operation Manual.
 * NOTE: Setting marked with e_ are considered expert settings and should be
 * used with caution and after consulting the  manual.
 */
struct Config
{
    Config()
    : bottomTrackPingsPerEnsemble(1)
    , maximumTrackingDepth(1000)
    , baudrate(BR9600)
    , parity(NONE)
    , stopBits(1)
    , automaticEnsembleCycling(true)
    , automaticPingCycling(true)
    , binaryDataOutput(true)
    , enableSerialOutput(true)
    , enableDataRecording(false)
    , headingAlignment(0)
    , e_headingBias(0)
    , salinity(35)
    , transformation(SHIP)
    , useTiltsInTransformation(true)
    , allow3BeamSolutions(true)
    , allowBinMapping(true)
    , speedOfSoundSource(EXTERNAL)
    , depthSource(EXTERNAL)
    , headingSource(EXTERNAL)
    , pitchAndRollSource(EXTERNAL)
    , salinitySource(EXTERNAL)
    , temperatureSource(EXTERNAL)
    , timePerEnsemble(base::Time().fromMilliseconds(0))
    , timeBetweenPings(base::Time().fromMilliseconds(50))
    , numberOfDepthCells(30)
    , pingsPerEnsemble(0)
    , depthCellSize(200)
    {
    }

    /**=====================
     * BOTTOM TRACK SETTINGS
     * =====================*/

    /**
     * The number of bottom-track pings to average together in each data
     * ensemble.
     * Range: 0 to 999
     */
    int bottomTrackPingsPerEnsemble;

    /**
     * The maximum tracking depth in bottom-track mode.
     * Range: 10 to 65535 decimeters
     * NOTE: Recommmended to set to a depth slightly greater than the
     * expected maximum depth.
     */
    int maximumTrackingDepth;


    /**=======================
     * CONTROL SYSTEM SETTINGS
     * =======================*/

    // SERIAL PORT CONTROL
    /**
     * Sets the baudrate of the DVL.
     */
    BAUDRATE baudrate;
    /**
     * Sets the parity of the DVL.
     */
    PARITY parity;
    /**
     * Stop Bits
     * Range: 1 or 2
     */
    int stopBits;

    // FLOW CONTROL
    /**
     * ENABLED: Automatically starts the next data collection cycle after the
     * current cycle is completed. Only a <BREAK> can stop this cycling.
     * DISABLED: Enters the STANDBY mode after transmission of the data
     * ensemble, displays the > prompt and waits for a new command.
     */
    bool automaticEnsembleCycling;
    /**
     * ENABLED: Pings immediately when ready.
     * DISABLED: Sends a > character to signal ready to ping, and then waits to
     * receive an <Enter> before pinging. The <Enter> sent to the ExplorerDVL
     * is not echoed.This feature lets you manually control ping timing within
     * the ensemble.
     */
    bool automaticPingCycling;
    /**
     * ENABLED: Sends the ensemble in binary format, if serial output is
     * enabled.
     * DISABLED: Sends the ensemble in readable hexadecimal-ASCII format, if
     * serial output is enabled.
     */
    bool binaryDataOutput;
    /**
     * ENABLED: Sends the data ensemble out the RS-232/422 serial interface.
     * DISABLED: No ensemble data are sent out the RS-232/422 interface.
     */
    bool enableSerialOutput;
    /**
     * No further documentation available, set to default.
     */
    bool enableDataRecording;


    /**======================
     * ENVIRONMENTAL SETTINGS
     * ======================*/

    /**
     * Corrects for physical misalignment between Beam 3 and the heading
     * reference.
     * Range: -17999 to 18000 hundreths of a degree
     */
    int headingAlignment;

    /**
     * Corrects for electrical/magnetic bias between the ExplorerDVL heading
     * value and the heading reference.
     * Range: -17999 to 18000 hundreths of a degree
     */
    int e_headingBias;

    /**
     * The water’s salinity value.
     * Range: 0 to 40 parts per thousand
     */
    int salinity;

    // COORDINATE TRANSFORMATION
    /**
     * Sets the Transformation Mode:
     *
     * <ul>
     *  <li>BEAM: No transformation. Radial beam coordinates.
     *      Heading/Pitch/Roll is not applied. Beam correction is not applied.
     *  <li>INSTRUMENT: Instrument coordinates. X, Y, Z vectors relative to the
     *      ExplorerDVL. Heading/Pitch/Roll not applied.
     *  <li>SHIP: Ship coordinates. X, Y, Z vectors relative to the ship.
     *      Heading not applied. If useTiltsInTransformation is true, then
     *      Pitch/Roll is applied.
     *  <li>EARTH: Earth coordinates. East, North and Vertical vectors relative
     *      to Earth. Heading applied. headingAlignment and e_headingBias used.
     *      If useTiltsInTransformation is true, then Pitch/Roll is applied.
     * </ul>
     *
     * NOTE: For ship and earth-coordinate transformations to work properly,
     * you must set headingAlignment and e_headingBias correctly. You also must
     * ensure that the tilt and heading sensors are active (see SENSOR SOURCE).
     */
    COORDINATE_SYSTEMS transformation;
    /**
     * If enabled, roll and pitch data is used in the transformation if SHIP or
     * EARTH is used as* transformation. Roll and pitch data collection is
     * unaffected by this setting.
     */
    bool useTiltsInTransformation;
    /**
     * If enabled allows a 3 beam solution should one beam be below the
     * correlation threshhold.
     */
    bool allow3BeamSolutions;
    /**
     * If enabeld allows the combination of data from beam sections in the same
     * depth of water. Does not account for pitch and roll.
     */
    bool allowBinMapping;

    // SENSOR SOURCE
    /**
     * Sets the source for the respective measurements.
     * <ul>
     *  <li>MANUAL: Uses a fixed manually set value.
     *  <li>INTERNAL: Uses an internal sensor.
     *  <li>EXTERNAL: Uses an external sensor.
     * </ul>
     */
    /* NOTE: INTERNAL: Calculates sos from depth, salinity and temperature. */
    SENSORSOURCE speedOfSoundSource;
    SENSORSOURCE depthSource;
    /* NOTE: INTERNAL is not allowed! */
    SENSORSOURCE headingSource;
    SENSORSOURCE pitchAndRollSource;
    /* NOTE: INTERNAL is not allowed! */
    SENSORSOURCE salinitySource;
    SENSORSOURCE temperatureSource;


    /**===============
     * TIMING SETTINGS
     * ===============*/

    /**
     * Minimum interval between data collection cycles (data ensembles).
     * Range: 0 to 24h59'59.99"
     */
    base::Time timePerEnsemble;
    /**
     * Minimum time between Pings.
     * Range: 0 to 59'59.99"
     */
    base::Time timeBetweenPings;


    /**========================
     * WATER PROFILING SETTINGS
     * ========================*/

    /**
     * Number of depth cells over which the ExplorerDVL collects data.
     * Range: 1 to 255
     */
    int numberOfDepthCells;
    /**
     * Number of pings to average in each data ensemble.
     * Range: 0 to 16384
     */
    int pingsPerEnsemble;
    /**
     * Height of one measurement cell.
     * Range: 10 to 800 cm
     */
    int depthCellSize;
};

}  // end namespace dvl_teledyne

#endif  // __DVL_TELEDYNE_CONFIG_HPP_
