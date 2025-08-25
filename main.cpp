#include<Hypodermic/Hypodermic.h>


#include "configuration/ContainerSetup.hpp"

int main() {
    const auto container = containerSetup();
    crow::SimpleApp app;

    // Bind all annotated routes
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    app.port(8080)
        // .multithreaded()
        .concurrency(3).run();
}
