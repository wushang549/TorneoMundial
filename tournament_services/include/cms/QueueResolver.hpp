//
// Created by tomas on 9/13/25.
//

#ifndef SERVICE_QUEUE_RESOLVER_HPP
#define SERVICE_QUEUE_RESOLVER_HPP

#include <Hypodermic/Hypodermic.h>
#include <memory>

#include "configuration/IResolver.hpp"
#include "cms/IQueueMessageProducer.hpp"

class QueueResolver : public IResolver<IQueueMessageProducer> {
    std::weak_ptr<Hypodermic::Container> container;
public:
    QueueResolver(const std::shared_ptr<Hypodermic::Container>& container) : container(std::move(container)) {}

    std::shared_ptr<IQueueMessageProducer> Resolve(const std::string_view& key) override {
        auto cont = container.lock();
        return cont->resolveNamed<QueueMessageProducer>(key.data());
    }

    std::shared_ptr<IQueueMessageProducer> Resolve() override{
        auto cont = container.lock();
        return cont->resolve<QueueMessageProducer>();
    }
};
#endif //SERVICE_QUEUE_RESOLVER_HPP