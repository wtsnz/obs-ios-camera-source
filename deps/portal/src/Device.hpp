#include <map>
#include <string>
#include <vector>
#include <iostream>

#include <usbmuxd.h>

// #include <boost/cstdint.hpp>
// #include <boost/shared_ptr.hpp>
#include "Protocol.hpp"
#include "Channel.hpp"

namespace portal
{
	///Forward reference so that we can be best friends
	class Portal;

    class DeviceDelegate {
    public:
        virtual void deviceDidConnect() = 0;
        virtual void deviceDidDisconnect() = 0;
        
        virtual ~DeviceDelegate() {};
    };
    
	/**
	Represents an iOS device detected by usbmuxd. This class
	can be used to retrieve info and initiate TCP sessions
	with the iOS device.
	*/
    class Device: public std::enable_shared_from_this<Device>
	{
	public:
        
        std::shared_ptr<Device> getptr() {
            return shared_from_this();
        }
        
		/**
		Default constructor for the class.
		*
		@param device The underlying usbmuxd device.
		*/
		explicit Device(const usbmuxd_device_info_t& device);
		Device(const Device& other);
		Device & operator=(const Device& rhs);
        
		/**
		Returns whether or not the device is connected
		*
		@return true if connected, otherwise false.
		*/
		bool isConnected() const;

		/**
		Gets the usbmuxd handle for the device. This handle can be
		used to comminucate directly with the device using libusbmuxd.
		*
		@return The usbmuxd handle
		*/
		int usbmuxdHandle() const;

		/**
		Returns the 40 character UUID associated with the device.
		*
		@return The devices UUID.
		*/
		const std::string & uuid() const;

		/**
		Returns the name of the product.
		*
		@return The print friendly version of the product name.
		*/
		// const std::string & productName() const;

		/**
		Returns the product ID. This is used to determine the product name.
		*
		@return The 16 bit product ID.
		*/
		uint16_t productID() const;

		int connect(uint16_t port, ChannelDelegate *channelDelegate);

		~Device();

		typedef std::map<std::string, std::vector<Device*> > DeviceMap;
        typedef std::vector<std::shared_ptr<Channel> > ChannelsVec;
        typedef std::shared_ptr<Device> shared_ptr;

        void setDelegate(DeviceDelegate *newDelegate)
        {
            delegate = newDelegate;
        }
        
	private:
		
        DeviceDelegate *delegate = nullptr;

		//typedef std::map<Device&, std::vector<Channel> > ChannelMap;
//        typedef std::map<uint16_t, std::string> ProductMap;
		
		///Keeps track of all devices associated by their uuid
		static DeviceMap s_devices;
		
        std::unique_ptr<Channel> connectedChannel;
        
        /// A list of all the channels for this device
//        static ChannelsVec s_connectedChannels;
        
		///Internal list of string values for products
//        static ProductMap s_products;

		bool _connected;
		usbmuxd_device_info_t _device;
		std::string _uuid;
		// std::string _productName;

		//Removes this device from the device list
		void removeFromDeviceList();

		friend class Peertalk;
		friend std::ostream & operator<<(std::ostream & os, const Device& v);
	};
}
