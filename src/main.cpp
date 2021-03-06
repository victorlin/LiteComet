#include <sstream>
#include <iostream>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <pion/net/WebServer.hpp>
#include <yaml.h>

#include "Config.hpp"
#include "ShutdownManager.hpp"
#include "Channel.hpp"
#include "CometReadService.hpp"
#include "CometWriteService.hpp"
#include "CometStatusService.hpp"

using namespace std;
using namespace boost::asio;
using namespace boost::posix_time;
using namespace pion;
using namespace pion::net;
using namespace YAML;
using namespace lite_comet;

int main(int argc, char* argv[])
{
    // Port number to run read server
    unsigned int read_port = 8080;
    // Network interface to run read server
    string read_interface = "0.0.0.0";
    // Port number to run write server
    unsigned int write_port = 9080;
    // Network interface to run write server
    string write_interface = "0.0.0.0";
    // Number of threads to run
    size_t numThreads = 1;

    // Initialize log system (use simple configuration)
    PionLogger main_log(PION_GET_LOGGER("lite_comet"));
    PionLogger pion_log(PION_GET_LOGGER("pion"));
    PION_LOG_SETLEVEL_INFO(main_log);
    PION_LOG_SETLEVEL_INFO(pion_log);
    PION_LOG_CONFIG_BASIC;

    string config_file_name = "config.yaml";

    namespace po = boost::program_options;
    po::options_description desc("Available options:");
    desc.add_options()
        ("help", "Print help message")
        ("config,c", 
            po::value<string>(&config_file_name)->default_value("config.yaml"), 
            "File path to configuration")
        ("debug", 
            po::value<bool>(), 
            "Whether to turn logger level to debug")
        ("read_port", 
            po::value<size_t>(), 
            "Port of read comet server")
        ("read_interface", 
            po::value<string>(), 
            "Interface of read comet server")
        ("write_port", 
            po::value<size_t>(), 
            "Port of write comet server")
        ("write_interface", 
            po::value<string>(), 
            "Interface of write comet server")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return 1;
    }

    PION_LOG_INFO(main_log, "Initialize Lite Comet server");
    PION_LOG_INFO(main_log, "Loading configuration from " << config_file_name);

    ifstream yaml_file(config_file_name.c_str());
    Parser parser(yaml_file);

    // Read the configuration yaml doc
    Node doc;
    parser.GetNextDocument(doc);

    // Debug logger level
    bool debug;
    if(const Node *debug_node= doc.FindValue("debug")) {
        *debug_node >> debug;
    }
    if (vm.count("debug")) {
        debug = vm["debug"].as<bool>();
    }
    if(debug) {
        PION_LOG_SETLEVEL_DEBUG(main_log);
        //PION_LOG_SETLEVEL_DEBUG(pion_log);
    }

    // Configuration for lite comet 
    if(const Node *lite_comet_node = doc.FindValue("lite_comet")) {
        PION_LOG_INFO(main_log, "Loading Lite Comet config");
        const Node *node = NULL;
        if((node = lite_comet_node->FindValue("NO_OFFSET"))) {
            *node >> Config::instance().NO_OFFSET;
        }
        if((node = lite_comet_node->FindValue("WAITING_DATA"))) {
            *node >> Config::instance().WAITING_DATA;
        }
        if((node = lite_comet_node->FindValue("NEEDS_RESYNC"))) {
            *node >> Config::instance().NEEDS_RESYNC;
        }
        if((node = lite_comet_node->FindValue("CHANNEL_TIMEOUT"))) {
            *node >> Config::instance().CHANNEL_TIMEOUT;
        }
        if((node = lite_comet_node->FindValue("CHANNEL_MAX_MSG"))) {
            *node >> Config::instance().CHANNEL_MAX_MSG;
        }
        if((node = lite_comet_node->FindValue("CLEANUP_CYCLE"))) {
            *node >> Config::instance().CLEANUP_CYCLE;
        }
    } else {
        PION_LOG_WARN(main_log, 
            "Can't find lite_comet setting in configuration file.");
    }

    // Configuration for server
    if(const Node *server_node = doc.FindValue("server")) {
        PION_LOG_INFO(main_log, "Loading server config");
        // Read port number
        const Node *node = NULL;
        if((node = server_node->FindValue("read_port"))) {
            *node >> read_port;
        }
        // Read interface
        if((node = server_node->FindValue("read_interface"))) {
            *node >> read_interface;
        }
        if((node = server_node->FindValue("write_port"))) {
            *node >> write_port;
        }
        // Read interface
        if((node = server_node->FindValue("write_interface"))) {
            *node >> write_interface;
        }
        // Read number of threads 
        if((node = server_node->FindValue("numThreads"))) {
            *node >> numThreads;
        }
    } else {
        PION_LOG_WARN(main_log, 
            "Can't find server setting in configuration file.");
    }

    if (vm.count("read_port")) {
        read_port = vm["read_port"].as<size_t>();
    }
    if (vm.count("read_interface")) {
        read_interface = vm["read_interface"].as<string>();
    }
    if (vm.count("write_port")) {
        write_port = vm["write_port"].as<size_t>();
    }
    if (vm.count("write_interface")) {
        write_interface = vm["write_interface"].as<string>();
    }

    try {
        // Address of interface
        const ip::address read_address(
            ip::address::from_string(read_interface));
        // Endpoint to listen reading server
        const ip::tcp::endpoint read_endpoint(read_address, read_port);

        // Address of interface
        const ip::address write_address(
            ip::address::from_string(write_interface));
        // Endpoint to listen writing server
        const ip::tcp::endpoint write_endpoint(write_address, write_port);

        // Cocurrency model
        PionOneToOneScheduler scheduler;
        scheduler.setNumThreads(numThreads);
        
        PION_LOG_INFO(main_log, "Read port: " << read_port);
        PION_LOG_INFO(main_log, "Read interface: " << read_interface);
        PION_LOG_INFO(main_log, "Write port: " << write_port);
        PION_LOG_INFO(main_log, "Write interface: " << write_interface);

        if(numThreads != 1) {
            PION_LOG_FATAL(main_log, "Only support 1 thread now");
            return 1;
        }
        PION_LOG_INFO(main_log, "Number of threads: " << numThreads);

        ChannelManager channel_manager(
            scheduler.getIOService(), 
            milliseconds(Config::instance().CLEANUP_CYCLE));
        channel_manager.startCleanup();

        CometReadService read_service(channel_manager);
        CometWriteService write_service(channel_manager);
        CometStatusService status_service(channel_manager);

        WebServer read_server(scheduler, read_endpoint);
        read_server.addService("/comet", 
            dynamic_cast<WebService *>(&read_service));
        read_server.addService("/cometStatus", 
            dynamic_cast<WebService *>(&status_service));

        WebServer write_server(scheduler, write_endpoint);
        write_server.addService("/comet", 
            dynamic_cast<WebService *>(&write_service));

        read_server.start();
        write_server.start();
        main_shutdown_manager.wait();
    } catch (std::exception& e) {
        PION_LOG_FATAL(main_log, e.what());
    }

    return 0;
}
