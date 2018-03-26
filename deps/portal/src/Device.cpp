#include "Device.hpp"
#include <list>
#include <sstream>
#include <vector>
// #include <boost/assign.hpp>

namespace portal
{

Device::DeviceMap Device::s_devices;
//Device::ChannelsVec Device::s_connectedChannels;

//Device::ChannelMap Device::channels;

// Device::ProductMap Device::s_products = 
// 	emplace(
// 		(0x1290, "iPhone")
// 		(0x1292, "iPhone 3G")
// 		(0x1294, "iPhone 3GS")
// 		(0x1297, "iPhone 4 GSM")
// 		(0x129c, "iPhone 4 CDMA")
// 		(0x12a0, "iPhone 4S")
// 		(0x12a8, "iPhone 5")
// 		(0x1291, "iPod touch")
// 		(0x1293, "iPod touch 2G")
// 		(0x1299, "iPod touch 3G")
// 		(0x129e, "iPod touch 4G")
// 		(0x129a, "iPad")
// 		(0x129f, "iPad 2 Wi-Fi")
// 		(0x12a2, "iPad 2 GSM")
// 		(0x12a3, "iPad 2 CDMA")
// 		(0x12a9, "iPad 2 R2")
// 		(0x12a4, "iPad 3 Wi-Fi")
// 		(0x12a5, "iPad 3 CDMA")
// 		(0x12a6, "iPad 3 Global")
// 		(0x129d, "Apple TV 2G")
// 		(0x12a7, "Apple TV 3G"));

Device::Device(const usbmuxd_device_info_t& device) : 
	_connected(true),
	_device(device),
	_uuid(_device.udid)
	//Lookup product name in the products map, If it does not exisit
	// then format a string with product_id. "Unknown Device (0x12a6)"
	// _productName(s_products.find(_device.product_id) == s_products.end() ? 
		// static_cast<std::ostringstream&>(std::ostringstream().seekp(0) << "Unknown Device (0x"  << std::hex << _device.product_id << ")").str() : 
		// s_products[_device.product_id])
{
	s_devices[_uuid].push_back(this);
	std::cout << "Added " << this << " to device list" << std::endl;
}

Device::Device(const Device& other) :
	_connected(other._connected),
	_device(other._device),
	_uuid(other._uuid)
	// _productName(other._productName)
{
	s_devices[_uuid].push_back(this);
	std::cout << "Added " << this << " to device list (copy)" << std::endl;
}

Device & Device::operator=(const Device& rhs)
{
	removeFromDeviceList();
	
	this->_connected = rhs._connected;
	this->_device = rhs._device;
	this->_uuid = rhs._uuid;
	// this->_productName = rhs._productName;

	s_devices[_uuid].push_back(this);

	return *this;
}

const std::string & Device::uuid() const
{
	return _uuid;
}

// const std::string & Device::productName() const
// {
// 	return _productName;
// }

int Device::usbmuxdHandle() const
{
	return _device.handle;
}

uint16_t Device::productID() const
{
	return _device.product_id;
}

int Device::connect(uint16_t port, ChannelDelegate *newChannelDelegate)
{
	int retval = 0;

	//TODO: Create a channel to manage the connection
	int conn = usbmuxd_connect(_device.handle, port);
    
    if (conn > 0) {
        
        // Add channel to channels
        
//        connectedChannel.reset(new Channel(port, conn));
        
//        connectedChannel = new Channel(port, conn);
//        connectedChannel->setDelegate(newChannelDelegate);
//        connectedChannel = std::make_shared<Channel>(port, conn);
        connectedChannel = std::unique_ptr<Channel>(new Channel(port, conn));
        connectedChannel->setDelegate(newChannelDelegate);
//        connectedChannel = connectedChannel.make_shared(port, conn);
        
//        connectedChannel->setDelegate(delegate);
        
//        connectedChannel->setDelegate(channelDelegate);
        
//        this->connectedChannel = channel;
//        this->connectedChannel = channel;
        
//        s_connectedChannels.push_back(channel);
        
    }
    
	return retval;
}

void Device::removeFromDeviceList()
{
	//Remove this device from the tracked devices
	std::vector<Device*>& devs = s_devices[_uuid];
	std::vector<Device*>::iterator it = std::find(devs.begin(), devs.end(), this);
	if(it != devs.end())
		devs.erase(it);
}

Device::~Device()
{
//    delete connectedChannel;
	removeFromDeviceList();

	std::cout << "Removed " << this << " from device list" << std::endl;
}

std::ostream & operator<<(std::ostream & os, const Device& v)
{
	os << "v.productName()" << " [ " << v.uuid() << " ]";
	return os;
}

}
