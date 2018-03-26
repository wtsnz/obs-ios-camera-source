#include <usbmuxd.h>
#include <vector>
#include <map>

#include "Device.hpp"

typedef void (*portal_channel_receive_cb_t) (char *buffer, int buffer_len, void *user_data);

namespace portal
{

    class PortalDelegate {
    public:
//        virtual void portalDeviceDidConnect() = 0;
//        virtual void portalDeviceDidDisconnect() = 0;
        
        virtual void portalDeviceDidReceivePacket(std::vector<char> packet) = 0;
        
        virtual ~PortalDelegate() {};
    };
    
class Portal: public ChannelDelegate, public std::enable_shared_from_this<Portal>
{
public:
	Portal();
    ~Portal();

    std::shared_ptr<Portal> getptr() {
        return shared_from_this();
    }
    
    int startListeningForDevices();
    void stopListeningForDevices();
    bool isListening();

    PortalDelegate *delegate;
    
private:
	typedef std::map<int, Device::shared_ptr> DeviceMap;

	bool _listening;
	Portal::DeviceMap _devices;

	Portal(const Portal& other);
	Portal& operator= (const Portal& other);

	void addDevice(const usbmuxd_device_info_t& device);
	void removeDevice(const usbmuxd_device_info_t& device);

	friend void pt_usbmuxd_cb(const usbmuxd_event_t *event, void *user_data);
    
    void channelDidReceivePacket(std::vector<char> packet);
    void channelDidStop();
    
};

// Namespace
}
