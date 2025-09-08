#ifndef DOMAIN_GROUP_HPP
#define DOMAIN_GROUP_HPP

#include <string>

namespace domain {
    class Group {
    private:
        /* data */
        std::string id;
        std::string name;
    public:
        inline Group(const std::string & name, const std::string & id = "") : id(std::move(id)), name(std::move(name)) {
        }
    };
}

#endif