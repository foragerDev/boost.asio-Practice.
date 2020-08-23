#pragma once
#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include "Acceptor.hpp"
using namespace boost::asio;
class Server {
public:
	Server() : m_stop(false) {}
	void Start(unsigned short port_num) {
		m_thread.reset(new std::thread([this, port_num] {
			Run(port_num);
			}));
	}

	void Stop() {
		m_stop.store(true);
		// here it waits for the thread to return started by the start method.
		m_thread->join();
	}

private:
	//this is used to run the server application. so server will stop when ever we will call stop function on the sever object.
	void Run(unsigned short port_num) {
		Acceptor acc(m_ios, port_num);
		while (!m_stop.load()) {
			acc.Accept();
		}
	}

	std::unique_ptr<std::thread> m_thread;
	std::atomic<bool> m_stop;
	io_service m_ios;
};