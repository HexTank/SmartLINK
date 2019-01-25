//
//  main.cpp
//  SmartLink
//
//  Created by Paul Tankard on 12/12/2016.
//
// clang++ -std=c++11 -I <path_to_ASIO_include_dir> main.cpp -o smartlink
//
//
#define ASIO_STANDALONE

#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include "asio.hpp"

using namespace std;

int usbBytesSent = 0;
unsigned char sentBuffer[128*1024];
void SendDataToSpectrum( asio::serial_port &serial, uint8_t code, uint16_t location, uint16_t length, const void* data)
{
	static char tmpbuffer[6 + 8192];
	unsigned char result = 0;
	tmpbuffer[0] = 0xfe;
	tmpbuffer[1] = code;
	tmpbuffer[2] = static_cast<uint8_t>(location);
	tmpbuffer[3] = static_cast<uint8_t>(location>>8);
	tmpbuffer[4] = static_cast<uint8_t>(length);
	tmpbuffer[5] = static_cast<uint8_t>(length>>8);
	memcpy(tmpbuffer + 6, data, length);
	
	asio::write(serial, asio::buffer(tmpbuffer, length + 6));
	while(result!=0xaa)
	{
		asio::read(serial, asio::buffer(&result, 1));
	}
	memcpy(sentBuffer+usbBytesSent, tmpbuffer, length+6);
	usbBytesSent += length + 6;

	
	cout << (int)code << " : " << length <<  " bytes" << endl;
}


int main(int argc, const char * argv[])
{

	if(argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " input-snap-file" << endl;
		return 1;
	}
		
	std::ifstream snapshotFile(argv[1], std::ios::binary | std::ios::ate);
	if(!snapshotFile.good())
	{
		std::cout << "Error opening file " << argv[1] << endl;
		return 1;
	}

	vector<char> snapshotData(snapshotFile.tellg());
	snapshotFile.seekg(0, ios::beg);
	snapshotFile.read(snapshotData.data(), snapshotData.size());
	

	chrono::time_point<chrono::steady_clock> start = chrono::steady_clock::now();

	asio::io_service io;
	asio::serial_port serial(io, "/dev/cu.usbmodemFD131");
	
	serial.set_option(asio::serial_port_base::baud_rate(115200));
	
	int snapshotIndex = 0;
	int spectrumAddress = 0x4000;

	// send register details.
	SendDataToSpectrum(serial, 0xa0, 0, 27, snapshotData.data());
	snapshotIndex += 27;

	const int blocksize = 8000;
	const int transferAmount = 48 * 1024;//
	// send game data.
	for (int block=0; block<transferAmount/blocksize; ++block)
	{
		SendDataToSpectrum(serial, 0xaa, spectrumAddress, blocksize, snapshotData.data() + snapshotIndex);
		snapshotIndex += blocksize;
		spectrumAddress += blocksize;
	}
	if(transferAmount%blocksize)
	{
		SendDataToSpectrum(serial, 0xaa, spectrumAddress, transferAmount%blocksize, snapshotData.data() + snapshotIndex);
	}
		
	// start game
	SendDataToSpectrum(serial, 0x80, 0, 0, snapshotData.data());

	chrono::time_point<chrono::steady_clock> end = chrono::steady_clock::now();
	chrono::milliseconds total = chrono::duration_cast<chrono::milliseconds>(end - start);
	cout << "Total transfer time : " << total.count() << "ms" << endl;
	cout << "     USB bytes sent : " << usbBytesSent << "bytes" << endl;
	
    return 0;
}
