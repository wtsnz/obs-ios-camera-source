
//#include <pthread.h>
#include <usbmuxd.h>
#include <thread>

#include "Protocol.hpp"

namespace portal
{

class ChannelDelegate {
public:
    virtual void channelDidReceivePacket(std::vector<char> packet) = 0;
    virtual void channelDidStop() = 0;
    virtual ~ChannelDelegate() {};
};
    
class Channel: public SimpleDataPacketProtocolDelegate
{
public:
    Channel(int port, int sfd);
    ~Channel();
    
//    typedef std::shared_ptr<Channel> shared_ptr;
    
    void simpleDataPacketProtocolDelegateDidProcessPacket(std::vector<char> packet);

    void setDelegate(ChannelDelegate *newDelegate)
    {
        delegate = newDelegate;
    }
    
private:
	int port;
    int conn;
    
    void setPacketDelegate(SimpleDataPacketProtocolDelegate  *newDelegate)
    {
        protocol.setDelegate(newDelegate);
    }
    
    SimpleDataPacketProtocol protocol;
    
//    SimpleDataPacketProtocol protocol;

    
    ChannelDelegate *delegate;

    bool running = false;

    bool StartInternalThread();
    void WaitForInternalThreadToExit();
    void StopInternalThread();
    void InternalThreadEntry();
    
    static void * InternalThreadEntryFunc(void * This) {
        
        ((portal::Channel *)This)->InternalThreadEntry();
        return NULL;
        
    }
    
//    pthread_t _thread;
    
    std::thread _thread;
};
    
}
    
