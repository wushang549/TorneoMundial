#include<Hypodermic/Hypodermic.h>


#include "configuration/ContainerSetup.hpp"
#include "configuration/ApplicationProperties.hpp"

int main() {
    const auto container = containerSetup();
    crow::SimpleApp app;

    // Bind all annotated routes
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    auto appConfig = container->resolve<ApplicationProperties>();

    app.port(appConfig->Port())
        .concurrency(appConfig->Concurrency())
        .run();
}
