//
// Created by developer on 11/14/25.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sstream>
#include <iostream>
#include <memory>

#define private public
#define protected public
#include "cms/MatchCreationListener.hpp"
#undef private
#undef protected

using ::testing::Test;

namespace {

    // Simple fixture in case you want to extend later
    struct MatchCreationListenerFixture : public Test {
        std::shared_ptr<ConnectionManager> connectionManager{nullptr};
    };

} // namespace

// Verifies that the listener can be constructed and destroyed without crashing.
TEST_F(MatchCreationListenerFixture, CanBeConstructedAndDestroyed) {
    MatchCreationListener listener(connectionManager);
    // Nothing to assert here; just ensure no crash / UB in ctor/dtor.
}

// Verifies that processMessage logs the expected prefix and payload to std::cout.
TEST_F(MatchCreationListenerFixture, ProcessMessage_LogsMessageWithPrefix) {
    // Capture std::cout
    std::ostringstream capture;
    auto* oldBuf = std::cout.rdbuf(capture.rdbuf());

    MatchCreationListener listener(connectionManager);

    const std::string payload = R"({"id":"match-123","round":"qf"})";
    listener.processMessage(payload);

    // Restore cout
    std::cout.rdbuf(oldBuf);

    const std::string output = capture.str();

    // We do a contains check to avoid being strict about newlines / extra spaces
    const std::string expected = "Match created: " + payload;
    EXPECT_NE(output.find(expected), std::string::npos)
        << "Output was:\n" << output << "\nExpected to contain:\n" << expected;
}
