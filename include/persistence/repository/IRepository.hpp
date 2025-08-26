//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_IREPOSITORY_HPP
#define RESTAPI_IREPOSITORY_HPP
#include <vector>
#include <memory>

template<typename Type, typename Id>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::shared_ptr<Type> ReadById(Id id) = 0;
    virtual Id Save (const Type & entity) = 0;
    virtual void Delete(Id id) = 0;
    virtual std::vector<std::shared_ptr<Type>> ReadAll() = 0;
};
#endif //RESTAPI_IREPOSITORY_HPP