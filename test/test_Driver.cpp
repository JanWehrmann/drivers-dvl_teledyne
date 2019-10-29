#include <boost/test/unit_test.hpp>
#include <dvl_teledyne/Driver.hpp>

namespace dvl_teledyne_test
{

class TestDriver : public dvl_teledyne::Driver
{
public:
    std::string testParseStandardCommand(const std::string& characters, int value, int numDigits, bool sign = false)
    {
        return parseStandardCommand(characters, value, numDigits, sign);
    }
};

}

BOOST_AUTO_TEST_SUITE(test_suite_DVLTeledyne_Driver)

BOOST_AUTO_TEST_CASE(test_parseStandardCommand)
{
    dvl_teledyne_test::TestDriver driver = dvl_teledyne_test::TestDriver();

    std::string characters = "$EN";
    int numDigits = 6;
    bool sign = true;
    int value = -215;

    std::string command = driver.testParseStandardCommand(characters, value, numDigits, sign);

    // Length of characters + numDigits + sign (0 or 1) + 1 (new line: "\n")
    size_t correctNumOfBytes = characters.length() + numDigits + static_cast<int>(sign) + 1;

    BOOST_REQUIRE_EQUAL(command.size(), correctNumOfBytes);
}
BOOST_AUTO_TEST_SUITE_END()
