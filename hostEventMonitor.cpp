#include "config.h"
#include "hostEventMonitor.hpp"
#include <phosphor-logging/log.hpp>

#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

static constexpr bool DEBUG = true;

namespace phosphor
{
namespace hostEvent
{

using namespace phosphor::logging;

void printConfig(HostEventConfig& cfg)
{
    std::cout << "Name: " << cfg.name << "\n";
    std::cout << "type: " << cfg.type << "\n";
    std::cout << "Freq: " << (int)cfg.freq << "\n";
    std::cout << "Window Size: " << (int)cfg.windowSize << "\n";
    std::cout << "Critical value: " << (int)cfg.criticalHigh << "\n";
    std::cout << "warning value: " << (int)cfg.warningHigh << "\n";
    std::cout << "Critical log: " << (int)cfg.criticalLog << "\n";
    std::cout << "Warning log: " << (int)cfg.warningLog << "\n";
    std::cout << "Critical Target: " << cfg.criticalTgt << "\n";
    std::cout << "Warning Target: " << cfg.warningTgt << "\n\n";
}

/** @brief Parsing HostEvent config JSON file  */
Json HostEventMon::parseConfigFile(std::string configFile)
{
    std::ifstream jsonFile(configFile);
    if (!jsonFile.is_open())
    {
        log<level::ERR>("config JSON file not found",
                        entry("FILENAME = %s", configFile.c_str()));
    }

    auto data = Json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        log<level::ERR>("config readings JSON parser failure",
                        entry("FILENAME = %s", configFile.c_str()));
    }

    return data;
}

void HostEventMon::getConfigData(Json& data, HostEventConfig& cfg)
{

    static const Json empty{};

    cfg.type = data.value("SensorType", "");
    /* Default frerquency of sensor polling is 1 second */
    cfg.freq = data.value("Frequency", 1);

    /* Default window size sensor queue is 1 */
    cfg.windowSize = data.value("Window_size", 1);

    auto threshold = data.value("Threshold", empty);
    cfg.criticalAlarm = false;
    cfg.warningAlarm = false;
    if (!threshold.empty())
    {
        auto criticalData = threshold.value("Critical", empty);
        if (!criticalData.empty())
        {
            cfg.criticalAlarm = true;
            cfg.criticalHigh = criticalData.value("Value", 0);
            cfg.criticalLog = criticalData.value("Log", true);
            cfg.criticalTgt = criticalData.value("Target", "");
        }
        auto warningData = threshold.value("Warning", empty);
        if (!warningData.empty())
        {
            cfg.warningAlarm = true;
            cfg.warningHigh = warningData.value("Value", 0);
            cfg.warningLog = warningData.value("Log", true);
            cfg.warningTgt = warningData.value("Target", "");
        }
    }
}

std::vector<HostEventConfig> HostEventMon::getEventConfig()
{

    std::vector<HostEventConfig> cfgs;
    HostEventConfig cfg;
    auto data = parseConfigFile(HOST_EVENT_CONFIG_FILE);

    // print values
    if (DEBUG)
        std::cout << "Config json data:\n" << data << "\n\n";

    /* Get CPU config data */
    for (auto& j : data.items())
    {
        auto key = j.key();

        HostEventConfig cfg = HostEventConfig();
        cfg.name = j.key();
        getConfigData(j.value(), cfg);
        cfgs.push_back(cfg);
        if (DEBUG)
            printConfig(cfg);

    }
    return cfgs;
}

void monitorHostStateService(){

    std::cout << "#### monitorHostStateService started\n";
    boost::asio::io_context io;
    auto conn= std::make_shared<sdbusplus::asio::connection>(io);
    static auto match = sdbusplus::bus::match::match(
    *conn,
    "type='signal',member='PropertiesChanged', "
    "path='/xyz/openbmc_project/state/host0', "
    "arg0='xyz.openbmc_project.State.Host'",
    [](sdbusplus::message::message& message) {
        std::string intfName;
        std::map<std::string, std::variant<std::string>> properties;
        std::string value;
        try
        {
            message.read(intfName, properties);
            if (properties.empty())
            {
                std::cerr << "ERROR: Empty PropertiesChanged signal received\n";
                return;
            }
            else
            {
                if (properties.begin()->first == "CurrentHostState")
                {
                    value = std::get<std::string>(properties.begin()->second);
                    std::cout << "CurrentHostState value:" << value <<  "\n";

                    boost::asio::io_context io;
                    auto conn= std::make_shared<sdbusplus::asio::connection>(io);
                    conn->async_method_call(
                    [](boost::system::error_code ec) {
                        if (ec)
                        {
                            std::cerr << "failed to set Value action\n";
                        }else 
                            std::cout << "async call to Properties. Set serialized via yield OK!\n";
                     },
                    "xyz.openbmc_project.HealthStatistics",
                    "/xyz/openbmc_project/sensors/oem/current_host_state",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Sensor.Value", "Value", std::variant<std::string>{value});
                }
                else if (properties.begin()->first == "RequestedHostTransition")
                {
                    value = std::get<std::string>(properties.begin()->second);
                    std::cout << "RequestedHostTransition value:" << value <<  "\n";

                    boost::asio::io_context io;
                    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
                    conn->async_method_call(
                    [](boost::system::error_code ec) {
                        if (ec)
                        {
                            std::cerr << "failed to set Value action\n";
                        }else 
                            std::cout << "async call to Properties. Set serialized via yield OK!\n";
                     },
                    "xyz.openbmc_project.HealthStatistics",
                    "/xyz/openbmc_project/sensors/oem/requested_host_transition",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Sensor.Value", "Value", std::variant<std::string>{value});
                }
            }     

        }
        catch (std::exception& e)
        {
            std::cerr << "Unable to read host state\n";
            return;
        }
        std::cout << "#### monitorHostStateService property change end\n";       
    });

}

void monitorRestartCauseService(){

    std::cout << "#### monitorRestartCauseService started\n";
    boost::asio::io_context io;
    auto conn= std::make_shared<sdbusplus::asio::connection>(io);
    static auto match = sdbusplus::bus::match::match(
    *conn,
    "type='signal',member='PropertiesChanged', "
    "path='/xyz/openbmc_project/control/host0/restart_cause', "
    "arg0='xyz.openbmc_project.Control.Host.RestartCause'",
    [](sdbusplus::message::message& message) {
        std::string intfName;
        std::map<std::string, std::variant<std::string>> properties;
        std::string value;
        try
        {
            message.read(intfName, properties);
            if (properties.empty())
            {
                std::cerr << "ERROR: Empty PropertiesChanged signal received\n";
                return;
            }
            else
            {
                if (properties.begin()->first == "RestartCause")
                {
                    value = std::get<std::string>(properties.begin()->second);
                    std::cout << "RestartCause value:" << value <<  "\n";

                    boost::asio::io_context io;
                    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
                    conn->async_method_call(
                    [](boost::system::error_code ec) {
                        if (ec)
                        {
                            std::cerr << "failed to set Value action\n";
                        }else 
                            std::cout << "async call to Properties. Set serialized via yield OK!\n";
                     },
                    "xyz.openbmc_project.HealthStatistics",
                    "/xyz/openbmc_project/sensors/oem/restart_cause",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Sensor.Value", "Value", std::variant<std::string>{value});
                }
            }     

        }
        catch (std::exception& e)
        {
            std::cerr << "Unable to read host state\n";
            return;
        }
        std::cout << "#### monitorRestartCauseService property change end\n";       
    });

}

/* Create dbus utilization sensor object for each configured sensors */
void HostEventMon::createHostEventSensors()
{

    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // test object server
    conn->request_name(HOST_EVENT_BUS_NAME);
    auto server = sdbusplus::asio::object_server(conn);
    //std::string host_event[4] = {"current_host_state", "requested_host_transition", "restart_cause", "Storage"};
    std::string ifaceobjpath = "";

    for (auto& cfg : sensorConfigs)
    {
        if (cfg.type == "utilization")
        {
            ifaceobjpath = "/xyz/openbmc_project/sensors/utilization/" + cfg.name;
        }
        else 
        {
            ifaceobjpath = "/xyz/openbmc_project/sensors/oem/" + cfg.name;
        }

        std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
            server.add_interface(ifaceobjpath, "xyz.openbmc_project.Sensor.Value");
    
        
        if (cfg.type == "utilization")
        {
            iface->register_property("Value", static_cast<double>(0),
                             sdbusplus::asio::PropertyPermission::readWrite);
            iface->register_property("Unit", std::string("xyz.openbmc_project.Sensor.Value.Unit.Percent"),
                             sdbusplus::asio::PropertyPermission::readWrite);

            std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceCritical = server.add_interface(ifaceobjpath, "xyz.openbmc_project.Sensor.Threshold.Critical");
            ifaceCritical->register_property("CriticalAlarmHigh", cfg.criticalAlarm, sdbusplus::asio::PropertyPermission::readWrite);
            ifaceCritical->register_property("CriticalHigh", cfg.criticalAlarm ? cfg.criticalHigh : static_cast<double>(0), sdbusplus::asio::PropertyPermission::readWrite);
            //no need CriticalAlarmLow
            ifaceCritical->register_property("CriticalAlarmLow", false, sdbusplus::asio::PropertyPermission::readWrite);
            ifaceCritical->register_property("CriticalLow", static_cast<double>(0), sdbusplus::asio::PropertyPermission::readWrite);
            ifaceCritical->initialize();

            std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceWarning =  server.add_interface(ifaceobjpath, "xyz.openbmc_project.Sensor.Threshold.Warning");
            ifaceWarning->register_property("WarningAlarmHigh", cfg.warningAlarm, sdbusplus::asio::PropertyPermission::readWrite);
            ifaceWarning->register_property("WarningHigh", cfg.warningAlarm ? cfg.warningHigh : static_cast<double>(0), sdbusplus::asio::PropertyPermission::readWrite);
            //no need WarningAlarmLow
            ifaceWarning->register_property("WarningAlarmLow", false, sdbusplus::asio::PropertyPermission::readWrite);
            ifaceWarning->register_property("WarningLow", static_cast<double>(0), sdbusplus::asio::PropertyPermission::readWrite);
            ifaceWarning->initialize();
        }
        else 
        {
            iface->register_property("Value", std::string("n/a"),
                             sdbusplus::asio::PropertyPermission::readWrite);
            iface->register_property("Unit", std::string("xyz.openbmc_project.Sensor.Value.Unit.Percent"), //no Unit for OEM
                             sdbusplus::asio::PropertyPermission::readWrite);
        }

        
        iface->initialize();
    }

    monitorHostStateService();
    monitorRestartCauseService();

    io.run();
}


} // namespace hostEvent
} // namespace phosphor

/**
 * @brief Main
 */
int main()
{

    double value = 0; 
    phosphor::hostEvent::HostEventMon hostEventMon(value);
    
    return 0;    
}
