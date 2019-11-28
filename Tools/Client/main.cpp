//
//  main.cpp
//  SmartLink
//
//  Created by Paul Tankard on 12/12/2016.
//
// ( GNUC) g++ -std=c++11 -I <path_to_ASIO_include_dir> main.cpp -o smartlink -lpthread
// (CLANG) clang++ -std=c++11 -I <path_to_ASIO_include_dir> main.cpp -o smartlink -lpthread
// ( MSVC) cl /EHsc /I <path_to_ASIO_include_dir> -D_WIN32_WINNT=0x0501 main.cpp /link /out:smartlink.exe
//
#define ASIO_STANDALONE

#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <deque>
#include <string>
#include "asio.hpp"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr uint8_t crc7_table[256] =
{
    0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
    0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
    0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
    0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
    0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
    0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
    0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
    0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
    0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
    0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
    0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
    0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
    0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
    0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
    0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
    0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
    0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
    0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
    0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
    0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
    0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
    0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
    0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
    0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
    0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
    0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
    0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
    0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
    0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
    0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
    0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
    0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

uint8_t crc7(const uint8_t *packet, size_t packet_len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < packet_len; i++)
    {
        crc = crc7_table[(crc << 1) ^ packet[i]];
    }
    return ((crc << 1) | 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class smartlink_serial
{
public:
    smartlink_serial(shared_ptr<asio::io_service> io_service, const string& device)
        : active_(true),
        io_service(io_service),
        serialPort(*io_service, device)
    {
        if (!serialPort.is_open())
        {
            std::cerr << "Failed to open serial port\n";
            return;
        }
        asio::serial_port_base::baud_rate baud_option(115200);
        asio::serial_port_base::flow_control flow_control(asio::serial_port::flow_control::none);
        serialPort.set_option(baud_option);
        serialPort.set_option(flow_control);

        read_start();
    }

    void write(shared_ptr<vector<uint8_t>> data)
    {
        io_service->post([this, data]()
        {
            do_write(data);
        });
    }

    void close()
    {
        io_service->post([this]()
        {
            do_close(asio::error_code());
        });
    }

    bool active()
    {
        return active_;
    }

private:

    static const int max_read_length = 16384;

    void read_start(void)
    {
        serialPort.async_read_some(asio::buffer(read_msg_, max_read_length), [=](const asio::error_code& error, size_t bytes_transferred)
        {
            cout << "Read : " << bytes_transferred << "bytes" << endl;
            if (!error)
            {
                if (bytes_transferred > 0 && read_msg_[0] == 0xaa && !write_msgs_.empty())
                {
                    cout << "Start next write" << endl;
                    write_start();
                }
                read_start();
            }
            else
                do_close(error);
        });
    }

    void do_write(shared_ptr<vector<uint8_t>> data)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(data);
        if (!write_in_progress)
            write_start();
    }

    void write_start(void)
    {
        cout << "Write : " << write_msgs_.front().get()->size() << "bytes" << endl;
        asio::async_write(serialPort, asio::buffer(write_msgs_.front().get()->data(), write_msgs_.front().get()->size()), [=](const asio::error_code& error, size_t bytes_transferred)
        {
            cout << "Wrote : " << bytes_transferred << "bytes" << endl;
            if (!error)
            {
                write_msgs_.pop_front();
                //				if (!write_msgs_.empty())
                //					write_start();
            }
            else
                do_close(error);
        });
    }

    void do_close(const asio::error_code& error)
    {
        if (error == asio::error::operation_aborted)
            return;
        if (error)
            cerr << "Error: " << error.message() << endl;
        else
            cout << "Error: Connection did not succeed.\n";
        serialPort.close();
        active_ = false;
    }

private:
    bool active_;
    shared_ptr<asio::io_service> io_service;
    asio::serial_port serialPort;
    uint8_t read_msg_[max_read_length];
    deque<shared_ptr<vector<uint8_t>>> write_msgs_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

shared_ptr<vector<uint8_t>> make_payload(uint8_t code, uint16_t location, uint16_t length)
{
    shared_ptr<vector<uint8_t>> payload(new vector<uint8_t>);
    payload->push_back('S' + 'L');
    payload->push_back(length + 6);
    payload->push_back((length + 6) >> 8);
    payload->push_back(crc7(payload->data(), payload->size()));
    payload->push_back(0xfe);
    payload->push_back(code);
    payload->push_back(location);
    payload->push_back(location >> 8);
    payload->push_back(length);
    payload->push_back(length >> 8);
    return payload;
}



vector<shared_ptr<vector<uint8_t>>> send_file(string file_to_load, int spectrumAddress)
{
    vector<shared_ptr<vector<uint8_t>>> payloads;

    ifstream file(file_to_load, ios::binary | ios::ate);
    vector<char> fileData(file.tellg());
    file.seekg(0, ios::beg);
    file.read(fileData.data(), fileData.size());

    int fileIndex = 0;
    shared_ptr<vector<uint8_t>> payload;

    const int blocksize = 9000;
    const int transferAmount = fileData.size();
    for (int block = 0; block < transferAmount / blocksize; ++block)
    {
        payload = make_payload(0xaa, spectrumAddress, blocksize);
        payload->insert(std::end(*payload), std::begin(fileData) + fileIndex, std::begin(fileData) + fileIndex + blocksize);
        payloads.push_back(payload);
        fileIndex += blocksize;
        spectrumAddress += blocksize;
    }
    if (transferAmount%blocksize)
    {
        payload = make_payload(0xaa, spectrumAddress, transferAmount%blocksize);
        payload->insert(std::end(*payload), std::begin(fileData) + fileIndex, std::begin(fileData) + fileIndex + (transferAmount%blocksize));
        payloads.push_back(payload);
    }
    return payloads;
}



vector<shared_ptr<vector<uint8_t>>> send_snapshot(string snapshot_to_load)
{
    vector<shared_ptr<vector<uint8_t>>> payloads;

    ifstream snapshotFile(snapshot_to_load, ios::binary | ios::ate);
    vector<char> snapshotData(snapshotFile.tellg());
    snapshotFile.seekg(0, ios::beg);
    snapshotFile.read(snapshotData.data(), snapshotData.size());


    int snapshotIndex = 0;
    int spectrumAddress = 0x4000;
    shared_ptr<vector<uint8_t>> payload;

    // register details payload
    payload = make_payload(0xa0, 0, 27);
    payload->insert(std::end(*payload), std::begin(snapshotData) + snapshotIndex, std::begin(snapshotData) + snapshotIndex + 27);
    payloads.push_back(payload);
    snapshotIndex += 27;

    // game data payloads
    const int blocksize = 9000;
    const int transferAmount = 48 * 1024;
    for (int block = 0; block < transferAmount / blocksize; ++block)
    {
        payload = make_payload(0xaa, spectrumAddress, blocksize);
        payload->insert(std::end(*payload), std::begin(snapshotData) + snapshotIndex, std::begin(snapshotData) + snapshotIndex + blocksize);
        payloads.push_back(payload);
        snapshotIndex += blocksize;
        spectrumAddress += blocksize;
    }
    if (transferAmount%blocksize)
    {
        payload = make_payload(0xaa, spectrumAddress, transferAmount%blocksize);
        payload->insert(std::end(*payload), std::begin(snapshotData) + snapshotIndex, std::begin(snapshotData) + snapshotIndex + (transferAmount%blocksize));
        payloads.push_back(payload);
    }

    // start game payload
    payload = make_payload(0x80, 0, 0);
    payloads.push_back(payload);

    return payloads;
}

shared_ptr<vector<uint8_t>> make_smartlink_action(uint16_t code)
{
    shared_ptr<vector<uint8_t>> payload(new vector<uint8_t>);
    payload->push_back('S' + 'L');
    payload->push_back(code);
    payload->push_back((code >> 8) | 0x80);
    payload->push_back(crc7(payload->data(), payload->size()));
    return payload;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char * argv[])
{
    try
    {
        if (argc != 2)
        {
            cerr << "Usage: " << argv[0] << " <device>\n";
            return 1;
        }

        shared_ptr<asio::io_service> io_service(new asio::io_service);
        shared_ptr<asio::io_service::work> work(new asio::io_service::work(*io_service));

        smartlink_serial c(io_service, argv[1]);
        thread t([&] { io_service->run(); });

        while (c.active())
        {
            // validation? Not yet, let's get the basics working :)
            string s;
            cin >> s;
            if (s == "q") break;
            if (s == "load")
            {
                string snap_to_load;
                cin >> snap_to_load;
                vector<shared_ptr<vector<uint8_t>>> payloads = send_snapshot(snap_to_load);
                for (auto payload : payloads)
                {
                    c.write(payload);
                }
            }
            if (s == "reset")
            {
                c.write(make_smartlink_action(1));
            }
            if (s == "data")
            {
                string file_to_load;
                int address;
                cin >> file_to_load >> address;
                vector<shared_ptr<vector<uint8_t>>> payloads = send_file(file_to_load, address);
                for (auto payload : payloads)
                {
                    c.write(payload);
                }
            }
        }
        c.close();
        t.join();
    }
    catch (exception& e)
    {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
