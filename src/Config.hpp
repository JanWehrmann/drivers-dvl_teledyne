#ifndef __DVL_TELEDYNE_CONFIG_HPP_
#define __DVL_TELEDYNE_CONFIG_HPP_

#include <base/Time.hpp>
#include <base/Angle.hpp>
#include <dvl_teledyne/PD0Messages.hpp>
#include <math.h>

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
    int bottomTrackPingsPerEnsemble = 1;

    /**
     * The maximum tracking depth in bottom-track mode.
     * Range: 1 to 6553.5 meter
     * NOTE: Recommmended to set to a depth slightly greater than the
     * expected maximum depth.
     */
    float maximumTrackingDepth = 100;


    /**======================
     * ENVIRONMENTAL SETTINGS
     * ======================*/

    /**
     * Corrects for physical misalignment between Beam 3 and the heading
     * reference.
     * Range: -PI + 1/18000 * PI to PI radians
     */
    base::Angle headingAlignment = base::Angle::fromRad(0.25 * M_PI);

    /**
     * Corrects for electrical/magnetic bias between the ExplorerDVL heading
     * value and the heading reference.
     * Range: -PI + 1/18000 * PI to PI radians
     */
    base::Angle e_headingBias = base::Angle::fromRad(0);

    /**
     * The water’s salinity value.
     * Range: 0 to 40 parts per thousand
     */
    int salinity = 19;

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
    COORDINATE_SYSTEMS transformation = INSTRUMENT;
    /**
     * If enabled, roll and pitch data is used in the transformation if SHIP or
     * EARTH is used as* transformation. Roll and pitch data collection is
     * unaffected by this setting.
     */
    bool useTiltsInTransformation = true;
    /**
     * If enabled allows a 3 beam solution should one beam be below the
     * correlation threshhold.
     */
    bool allow3BeamSolutions = true;
    /**
     * If enabeld allows the combination of data from beam sections in the same
     * depth of water. Does not account for pitch and roll.
     */
    bool allowBinMapping = false;

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
    SENSORSOURCE speedOfSoundSource = EXTERNAL;
    SENSORSOURCE depthSource = EXTERNAL;
    /* NOTE: INTERNAL is not allowed! */
    SENSORSOURCE headingSource = EXTERNAL;
    SENSORSOURCE pitchAndRollSource = EXTERNAL;
    /* NOTE: INTERNAL is not allowed! */
    SENSORSOURCE salinitySource = EXTERNAL;
    SENSORSOURCE temperatureSource = EXTERNAL;


    /**===============
     * TIMING SETTINGS
     * ===============*/

    /**
     * Minimum interval between data collection cycles (data ensembles).
     * Range: 0 to 89999.99 s
     */
    base::Time timePerEnsemble = base::Time().fromMilliseconds(0);
    /**
     * Minimum time between Pings.
     * Range: 0 to 89999.99 s
     */
    base::Time timeBetweenPings = base::Time().fromMilliseconds(200);


    /**========================
     * WATER PROFILING SETTINGS
     * ========================*/

    /**
     * Number of depth cells over which the ExplorerDVL collects data.
     * Range: 1 to 255
     */
    int numberOfDepthCells = 30;
    /**
     * Number of pings to average in each data ensemble.
     * Range: 0 to 16384
     */
    int pingsPerEnsemble = 0;
    /**
     * Height of one measurement cell.
     * Range: 0.1 to 8 m
     */
    float depthCellSize = 2;
};

}  // end namespace dvl_teledyne

#endif  // __DVL_TELEDYNE_CONFIG_HPP_
