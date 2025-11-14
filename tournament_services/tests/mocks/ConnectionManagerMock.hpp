#pragma once

#include <gmock/gmock.h>
#include <memory>
#include <string_view>

#include "cms/ConnectionManager.hpp"

class ConnectionManagerMock : public ConnectionManager {
public:
    // Base methods are not virtual, so we do NOT use 'override' here.

    MOCK_METHOD(void,
                initialize,
                (std::string_view brokerURI,
                 std::string_view username,
                 std::string_view password,
                 std::string_view clientId));

    MOCK_METHOD(void,
                shutdown,
                ());

    MOCK_METHOD(std::shared_ptr<cms::Connection>,
                Connection,
                (),
                (const));

    MOCK_METHOD(std::shared_ptr<cms::Session>,
                CreateSession,
                (),
                (const));
};
