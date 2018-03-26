// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <fcntl.h>
// #include <stddef.h>
// #include <errno.h>
#include <chrono>
#include <thread>
// #ifdef WIN32
// #include <winsock2.h>
// #include <windows.h>
// typedef unsigned int socklen_t;
// #else
// #include <sys/socket.h>
// #include <sys/un.h>
// #include <arpa/inet.h>
// #include <pthread.h>
// #include <netinet/in.h>
// #include <iostream>
// #include <signal.h>
// #endif

// #include <socket.h>
// #include <usbmuxd.h>


// #include <iostream>
// #include <stdio.h>
// #include <usbmuxd.h>

// #define USBMUXD_SOCKET_PORT 27015

// static uint16_t listen_port = 2345;
// static uint16_t device_port = 0;

// struct client_data {
// 	int fd;
// 	int sfd;
// 	volatile int stop_ctos;
// 	volatile int stop_stoc;
// };

// /**
// usbmuxd callback
// */
// void pt_usbmuxd_cb(const usbmuxd_event_t *event, void *user_data)
// {
// 	// Peertalk *client = static_cast<Peertalk*>(user_data);

// 	switch(event->event)
// 	{
// 		case UE_DEVICE_ADD:
//             printf("Added a device");
// 			// client->addDevice(event->device);
// 		break;
// 		case UE_DEVICE_REMOVE:
//         printf("removed a device");
// 			// client->removeDevice(event->device);
// 		break;
// 	}
// }


#include <iostream>
#include <signal.h>
#include "Portal.hpp"

bool m_running = true;

void sig_interrupt(int sig)
{
	m_running = false;
}

int main()
{
    
    std::cout << "Looking for devices" << std::endl;
    std::shared_ptr<portal::Portal> client = std::make_shared<portal::Portal>();

    client->startListeningForDevices();
    signal(SIGINT, sig_interrupt);

    while(m_running && client->isListening())
    {
        //waiting for device
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Done!" << std::endl;

	return 0;
}
