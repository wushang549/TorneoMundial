#ifndef TOURNAMENTS_APPLICATION_PROPERTIES_HPP
#define TOURNAMENTS_APPLICATION_PROPERTIES_HPP
#include <nlohmann/json.hpp>

namespace config{
    struct ApplicationProperties
    {
        ApplicationProperties(int port, int concurrency) : port(port), concurrency(concurrency){}

        int const Port() {
            return port;
        }

        int const Concurrency() {
            return concurrency;
        }
        
        // private:
        int port;
        int concurrency;
    };

    void to_json(json& j, const ns::person& p)
{
    j = json{ {"name", p.name}, {"address", p.address}, {"age", p.age} };
}
void from_json(const json& j, ns::person& p)
{
    j.at("name").get_to(p.name);
    j.at("address").get_to(p.address);
    j.at("age").get_to(p.age);
}
}
#endif
