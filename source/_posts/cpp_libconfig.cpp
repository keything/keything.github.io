#include <libconfig.h++>
#include <iostream>

int main() {
    try {
        libconfig::Config config;
        config.readFile("config.ini");
        const libconfig::Setting& serverConf = config.lookup("server");
        int serverPort;
        bool flag = serverConf.lookupValue("service_port", serverPort);
        std::cout << "server port " << serverPort << std::endl;
    } catch (const libconfig::ConfigException& e) {
        std::cout << "exception:" << e.what() << std::endl;
    }
}
