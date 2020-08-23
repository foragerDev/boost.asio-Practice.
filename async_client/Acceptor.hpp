#pragma once
#include <boost/predef.h>
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0601
#endif

#include "sync_tcp.hpp"
#include <boost/asio.hpp>

using namespace boost::asio;


class Acceptor {
public:
	Acceptor(io_service& ios, unsigned short port_num ) :
		m_ios(ios),
		m_acceptor(ip::tcp::endpoint(
			ip::address_v4::any(),
			port_num
		))
	{
		m_acceptor.listen();
	}
	void Accept() {
		ip::tcp::socket sock(m_ios);
		m_acceptor.accept(sock);
		Service svc;
		svc.HandleClient(sock);
	}

private:
	io_service& m_ios;
	ip::tcp::acceptor m_acceptor;
};