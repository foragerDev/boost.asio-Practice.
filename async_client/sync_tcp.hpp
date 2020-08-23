#pragma once

#include <boost/predef.h>
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0601
#endif

#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include <iostream>

using namespace boost::asio;
// This is the service that the server will provide the the client. And I will see when 
// it will be used in the main function. so lets do it.

class Service {
public:
	Service() {}
	void HandleClient(ip::tcp::socket& sock) {
		try {

			// this will do something here in this block of code. so Later if you wanna implement something that will perfom something, do it here. OK!
			streambuf request;
			read_until(sock, request, '\n');
			init = 0;
			while (init != 10000000) {
				init++;
				std::this_thread::sleep_for(std::chrono::microseconds(500));

				std::string response = "Response\n";
				write(sock, buffer(response));
			}
		}
		catch (boost::system::system_error& ec) {
			std::cout << "Error Occured! ErrorCode = " << ec.code() << ". Message: " << ec.what();
		}
	}
private:
	int init;
};