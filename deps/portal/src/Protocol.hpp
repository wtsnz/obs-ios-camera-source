#ifndef PEERTALK_PROTOCOL_H
#define PEERTALK_PROTOCOL_H

#include <vector>

class SimpleDataPacketProtocol;

namespace portal
{

    class SimpleDataPacketProtocolDelegate {
    public:
        virtual void simpleDataPacketProtocolDelegateDidProcessPacket(std::vector<char> packet) = 0;
        virtual ~SimpleDataPacketProtocolDelegate() {};
    };
    
class SimpleDataPacketProtocol
{
public:
    
    SimpleDataPacketProtocol();
    ~SimpleDataPacketProtocol();
    
    // Encode?
    
    int processData(char *data, int dataLength);
    
    void reset();
    
    void setDelegate(SimpleDataPacketProtocolDelegate *newDelegate) {
        delegate = newDelegate;
    }
    
private:
    
    SimpleDataPacketProtocolDelegate *delegate;
    
    std::vector<char> buffer;
    
};

}

#endif
