#ifndef C6CC4486_5E24_4F04_A7E2_71AFC7543E1C
#define C6CC4486_5E24_4F04_A7E2_71AFC7543E1C

#include <string_view>

class IMessageProducer
{
public:
    virtual ~IMessageProducer() = default;
    virtual void SendMessage(const std::string_view& message) = 0;
};
 

#endif /* C6CC4486_5E24_4F04_A7E2_71AFC7543E1C */
