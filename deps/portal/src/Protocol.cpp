#include "Protocol.hpp"

#include <cstdint>
#include <iostream>

#ifdef WIN32
    #include <winsock2.h>
#endif
namespace portal
{

    SimpleDataPacketProtocol::SimpleDataPacketProtocol() {
//        printf("%s: Init\n", __func__);
        std::cout << "SimpleDataPacketProtocol created\n";
    }
    
    SimpleDataPacketProtocol::~SimpleDataPacketProtocol() {
        buffer.clear();
        std::cout << "SimpleDataPacketProtocol destroyed\n";
    }
    
    int SimpleDataPacketProtocol::processData(char *data, int dataLength) {
        
        printf("%s: Init\n", __func__);
        
        if (this == nullptr) {
            printf("For some reason the simple data packet protocol doesn't exist");
            return -1;
        }
        
        if (dataLength > 0) {
            // Add data recieved to the end of buffer.
            buffer.insert(buffer.end(), data, data + dataLength);
        }
        
        uint32_t length = 0;
        // Ensure that the data inside the buffer is at least as big
        // as the length variable (32 bit int)
        // and then read it out
        if (buffer.size() < sizeof(length)) {
            
        } else {
            
            // Grab the length value out
            memcpy(&length, &buffer[0], sizeof length);
            length = ntohl(length);
            
            while (sizeof(length) + length <= buffer.size())
            {
                // Read length bytes as that is the packet
                std::vector<char>::const_iterator first = buffer.begin() + sizeof(length);
                std::vector<char>::const_iterator last = buffer.begin() + sizeof(length) + length;
                std::vector<char> newVec(first, last);
                
                printf("Got packet: %i\n", length);
                
//                std::shared_ptr<SimpleDataPacketProtocolDelegate> delegateReference = this->delegate.lock();
                
                if (delegate != nullptr) {
                    delegate->simpleDataPacketProtocolDelegateDidProcessPacket(newVec);
                }
//                if (this->delegate != nullptr) {
//                    this->delegate->simpleDataPacketProtocolDelegateDidProcessPacket(newVec);
//                }
                
                // Remove the data from buffer
                buffer.erase(buffer.begin(), buffer.begin() + sizeof(length) + length);
                
                // Read the next length
                memcpy(&length, &buffer[0], sizeof length);
                length = ntohl(length);
            }
            
        }
        
        return 0;
    }

}
