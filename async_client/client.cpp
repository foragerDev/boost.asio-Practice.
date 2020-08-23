#include "client.hpp"
#include <chrono>

int main() {
	try {
		AsyncTCPClient client;

		client.emulateLongComputationOp(10, "127.0.0.1", 3333, 1, handler);
		std::this_thread::sleep_for(std::chrono::seconds(10));
		client.emulateLongComputationOp(12, "127.0.0.1", 3334, 2, handler);
		client.cancelRequest(1);

		std::this_thread::sleep_for(std::chrono::seconds(6));
		client.emulateLongComputationOp(12, "127.0.0.1", 3335, 3, handler);
		std::this_thread::sleep_for(std::chrono::seconds(20));
		client.close();
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what();
		return e.code().value();
	}
	return 0;
}