#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <string>

namespace phosphor
{
namespace hostEvent
{

using Json = nlohmann::json;

struct HostEventConfig
{
    std::string name;
    std::string type;
    uint16_t freq;
    uint16_t windowSize;
    double criticalHigh;
    double warningHigh;
    bool criticalLog;
    bool warningLog;
    std::string criticalTgt;
    std::string warningTgt;
    bool criticalAlarm;
    bool warningAlarm;
};

class HostEventMon
{
  public:
    //HostEventMon() = delete;
    HostEventMon(const HostEventMon&) = delete;
    HostEventMon& operator=(const HostEventMon&) = delete;
    HostEventMon(HostEventMon&&) = delete;
    HostEventMon& operator=(HostEventMon&&) = delete;
    virtual ~HostEventMon() = default;

    /** @brief Constructs EventMo
     */
    HostEventMon(const double value)
    {
        // read json file
        sensorConfigs = getEventConfig();
        createHostEventSensors();
    }

     /** @brief Parsing HostEvent config JSON file  */
    Json parseConfigFile(std::string configFile);

    /** @brief reading config for each hostEvent sensor component */
    void getConfigData(Json& data, HostEventConfig& cfg);

    /** @brief Create sensors for hostEvent monitoring */
    void createHostEventSensors();

  private:
    std::vector<HostEventConfig> sensorConfigs;
    std::vector<HostEventConfig> getEventConfig();
};

} // namespace hostEvent
} // namespace phosphor
