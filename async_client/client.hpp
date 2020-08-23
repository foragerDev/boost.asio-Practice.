#pragma once

#include <boost/predef.h>
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0601
#endif

#include <boost/asio.hpp>

#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <string>
#include <boost/core/noncopyable.hpp>

using namespace boost::asio;


typedef void(*Callback) (
	unsigned int request_id,
	const std::string& response,
	const boost::system::error_code& ec);

void handler(unsigned int request_id, const std::string& response, const boost::system::error_code& ec) {
	if (ec == boost::system::errc::success) {
		std::cout << "Request # " << request_id
			<< " Has completed. Resposne: "
			<< response << std::endl;
	}
	else if (ec == error::operation_aborted) {
		std::cout << "Request #" << request_id
			<< " has been cancelled by the user." << std::endl;
	}
	else {
		std::cout << "Request #" << request_id
			<< " failed! Error code = " << ec.value()
			<< ". Error message = " << ec.message() << std::endl;
	}
	return;
}

struct Session {
	Session(io_service& ios,
		const std::string& raw_ip_address,
		unsigned short port_num,
		const std::string& request,
		unsigned int id,
		Callback callback) :
			m_sock(ios),
			m_ep(ip::address::from_string(raw_ip_address), port_num),
			m_request(request),
			m_id(id),
			m_callback(callback),
			m_was_cancelled(false)
		{
		// This will stay empty;
		}


	ip::tcp::socket m_sock;
	ip::tcp::endpoint m_ep;
	std::string m_request;

	streambuf m_response_buf;
	std::string m_response;

	boost::system::error_code m_ec;
	unsigned int m_id;

	Callback m_callback;
	bool m_was_cancelled;
	std::mutex m_cancel_guard;
};

class AsyncTCPClient : private boost::noncopyable {
public:
	AsyncTCPClient() {
		m_work.reset(new io_service::work(m_ios));
		m_thread.reset(new std::thread([this]() {
			m_ios.run();
			}));
	}

	void emulateLongComputationOp(
		unsigned int duration_sec,
		const std::string& raw_ip_address,
		unsigned short port_num,
		unsigned int request_id,
		Callback callback) {


		std::string request = "EMULATE_LONG_CALC_OP " + std::to_string(duration_sec) + "\n";

		std::shared_ptr<Session> session = std::shared_ptr<Session>(new Session(m_ios,
			raw_ip_address,
			port_num,
			request,
			request_id,
			callback
		));


		session->m_sock.open(session->m_ep.protocol());
		std::unique_lock<std::mutex> lock(m_active_sessions_guard);
		m_active_sockets[request_id] = session;
		lock.unlock();

		session->m_sock.async_connect(session->m_ep,
			[this, session](const boost::system::error_code& ec) {
				if (!ec) {
					session->m_ec = ec;
					onRequestComplete(session);
					return;
				}
				std::unique_lock<std::mutex> cancel_lock(session->m_cancel_guard);
				if (session->m_was_cancelled) {
					onRequestComplete(session);
					return;
				}
				async_write(session->m_sock,
					buffer(session->m_request),
					[this, session](const boost::system::error_code& ec, std::size_t bytes_transferred) {
						if (!ec) {
							session->m_ec = ec;
							onRequestComplete(session);
							return;
						}
						std::unique_lock<std::mutex> cancel_lock(session->m_cancel_guard);
						if (session->m_was_cancelled) {
							onRequestComplete(session);
							return;
						}
						async_read_until(session->m_sock, session->m_response_buf, "\n",
							[this, session](const boost::system::error_code& ec, std::size_t bytes_transferred) {
								if (!ec) {
									session->m_ec = ec;
								}
								else {
									std::istream strm(&session->m_response_buf);
									std::getline(strm, session->m_response);
								}
								onRequestComplete(session);
							});
					});
			});
	}
	void cancelRequest(unsigned int request_id) {
		std::unique_lock < std::mutex> lock(m_active_sessions_guard);

		auto it = m_active_sockets.find(request_id);
		if (it != m_active_sockets.end()) {
			std::unique_lock<std::mutex> cancel_lock(it->second->m_cancel_guard);
			it->second->m_was_cancelled = true;
			it->second->m_sock.cancel();
		}
	}
	void close() {
		m_work.reset(NULL);
		m_thread->join();
	}

private:
	io_service m_ios;
	std::map<int, std::shared_ptr<Session>> m_active_sockets;
	std::mutex m_active_sessions_guard;
	std::unique_ptr<io_service::work> m_work;
	std::unique_ptr <std::thread> m_thread;

	void onRequestComplete(std::shared_ptr<Session> session) {
		boost::system::error_code ignored_ec;
		session->m_sock.shutdown(
			ip::tcp::socket::shutdown_both,
			ignored_ec
			);
		std::unique_lock<std::mutex> lock(m_active_sessions_guard);
		auto it = m_active_sockets.find(session->m_id);
		if (it != m_active_sockets.end()) {
			m_active_sockets.erase(it);
		}
		lock.unlock();
		boost::system::error_code ec;

		if (session->m_ec == boost::system::errc::success && session->m_was_cancelled)
			ec = error::operation_aborted;
		else
			ec = session->m_ec;
		session->m_callback(session->m_id,
			session->m_response, ec);
	}
};