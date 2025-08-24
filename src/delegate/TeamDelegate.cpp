//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<persistence::Team, std::string> > repository) : teamRepository(repository) {
}
