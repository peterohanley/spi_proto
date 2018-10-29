//g++ click-test.cpp spi_proto_master.cpp spi_proto_lib/spi_chunks.cpp -std=c++14 -pthread -x c binary_semaphore.c -x c spi_proto_lib/spi_proto.c -I. -Ispi_proto_lib/ -x c crc16.c -x c spi_remote_host.c -o click -g
//causes solenoids to click in strobing sequence. Remote API test
#include <thread>
#include "spi_datagram.h"

using namespace spi_proto;

void
blip_task(void);
void (*spi_callback)(struct spi_packet *p) = NULL;

int main(int argc, char *argv[]) {
	std::thread remote_thread(datagram_task);
	std::thread click_thread(blip_task);
	
	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	
	return 0;
	
}

//functions implemented for port
void
delay_ms(unsigned int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void
blip_task(void)
{
	//forever:
	//	send heartrate 60
	//	wait 10s
	//	send heartrate 120
	//	wait 10s

	int wait = 10*1000;
	puts("starting blinking!");
	for (;;) {
		puts("blink slow");
		uint8_t buf[1] = {60};
		send_message(buf,1);
		delay_ms(wait);
		puts("blink fast");
		uint8_t buf2[1] = {120};
		send_message(buf2,1);
		delay_ms(wait);
	}
}
