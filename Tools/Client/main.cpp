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
#include <mutex>
#include <condition_variable>
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

#ifdef _MSC_VER
    #pragma pack(push)
    #pragma pack(1)
#endif
struct SNA_Regs
{
    uint8_t i;
    uint16_t _hl;
    uint16_t _de;
    uint16_t _bc;
    uint16_t _af;
    uint16_t hl;
    uint16_t de;
    uint16_t bc;
    uint16_t iy;
    uint16_t ix;
    uint8_t iff2 : 1;
    uint8_t ei : 1;
    uint8_t r;
    uint16_t af;
    uint16_t sp;
    uint8_t im;
    uint8_t border;
}
#ifdef __GNUC__
    __attribute((packed))
#endif
;

struct SmartLINK_Regs : SNA_Regs
{
    uint16_t pc;
}
#ifdef __GNUC__
    __attribute((packed))
#endif
;


struct Z80_V1_Shared_Regs
{
    uint8_t a;
    uint8_t f;
    uint16_t bc;
    uint16_t hl;
    uint16_t pc;    // 0 if v2 or v3
    uint16_t sp;
    uint8_t i;
    uint8_t r;
    uint8_t r_bit7 : 1;
    uint8_t border : 3;
    uint8_t samrom : 1;
    uint8_t compressed : 1;
    uint16_t de;
    uint16_t _bc;
    uint16_t _de;
    uint16_t _hl;
    uint8_t _a;
    uint8_t _f;
    uint16_t iy;
    uint16_t ix;
    uint8_t ei;
    uint8_t iff2;
    uint8_t im : 3;
}
#ifdef __GNUC__
    __attribute((packed))
#endif
;

struct Z80_V2_Shared_Regs : Z80_V1_Shared_Regs
{
    uint16_t headerSize;
    uint16_t _pc;
    uint8_t hardwareMode;
    uint8_t out_0x7ffd;
    uint8_t interface1Paged;
    uint8_t hwModify;
    uint8_t out_0xfffd;
    uint8_t ayRegs[16];
}
#ifdef __GNUC__
    __attribute((packed))
#endif
;

struct Z80_V3_Shared_Regs : Z80_V2_Shared_Regs
{
    uint16_t tStateLow;
    uint8_t tStateHigh;
    uint8_t pad;
    uint8_t mgtPaged;
    uint8_t multifacePaged;
    uint8_t rom1st8k;
    uint8_t rom2nd8k;
    uint16_t joystickMappings[5];
    uint16_t namesForMappings[5];
    uint8_t mgtType;
    uint8_t diciplePad1;
    uint8_t diciplePad2;
}
#ifdef __GNUC__
    __attribute((packed))
#endif
;

struct Z80_V3_1_Shared_Regs : Z80_V3_Shared_Regs
{
    uint8_t out_0x1ffd;
}
#ifdef __GNUC__
    __attribute((packed))
#endif
;

struct Z80_V1_Regs : Z80_V1_Shared_Regs
{
    uint8_t data[1];
}
#ifdef __GNUC__
__attribute((packed))
#endif
;

struct Z80_V2_Regs : Z80_V2_Shared_Regs
{
    uint8_t data[1];
}
#ifdef __GNUC__
__attribute((packed))
#endif
;

struct Z80_V3_Regs : Z80_V3_Shared_Regs
{
    uint8_t data[1];
}
#ifdef __GNUC__
__attribute((packed))
#endif
;

struct Z80_V3_1_Regs : Z80_V3_1_Shared_Regs
{
    uint8_t data[1];
}
#ifdef __GNUC__
__attribute((packed))
#endif
;

struct Z80_Page_Chunk_Header
{
    uint16_t compressedDataSize;
    uint8_t pageNumber;
    uint8_t data[1];
}
#ifdef __GNUC__
__attribute((packed))
#endif
;



#ifdef _MSC_VER
    #pragma pack(pop)
#endif

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
        : active_(true)
        , transferring_(false)
        , io_service(io_service)
        , serialPort(*io_service, device)
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

    template <typename DATA>
    void write(DATA data)
    {
        auto start = chrono::steady_clock::now();
        std::cerr << "Started writing\n";
        unique_lock<mutex> m(transfer_mutex);
        transferring_ = true;

        io_service->post([this, data]()
        {
            do_write(data);
        });

        transfer_waiting.wait(m, [&] { return !transferring_; });
        auto end = chrono::steady_clock::now();
        auto diff = end - start;
        std::cerr << "Finished writing : " << chrono::duration <double, milli>(diff).count() << " ms" << endl;
    }

    void close()
    {
        io_service->post([this]()
        {
            do_close(asio::error_code());
        });
    }

    void wait()
    {
    }

    bool active()
    {
        return active_;
    }

private:

    static const int max_read_length = 16384;

    void read_start(void)
    {
        serialPort.async_read_some(asio::buffer(read_msg_, max_read_length), [&](const asio::error_code& error, size_t bytes_transferred)
        {
            //cout << "Read : " << bytes_transferred << "bytes" << endl;
            if (!error)
            {
                if (bytes_transferred > 0 && read_msg_[0] == 0xaa && !write_msgs_.empty())
                {
                    //cout << "Start next write" << endl;
                    write_start();
                }
                if (write_msgs_.empty())
                {
                    //std::cout << "All done!" << std::endl;
                    unique_lock<mutex> m(transfer_mutex, std::try_to_lock);
                    transferring_ = false;
                    m.unlock();
                    transfer_waiting.notify_one();
                }
                read_start();
            }
            else
                do_close(error);
        });
    }

    void do_write(vector<shared_ptr<vector<uint8_t>>> data)
    {
        const bool write_in_progress = !write_msgs_.empty();
        for (auto &d : data)
        {
            write_msgs_.push_back(d);
        }
        if (!write_in_progress)
        {
            write_start();
        }
    }

    void do_write(shared_ptr<vector<uint8_t>> data)
    {
        const bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(data);
        if (!write_in_progress)
        {
            write_start();
        }
    }


    void write_start(void)
    {
        //cout << "Write : " << write_msgs_.front().get()->size() << "bytes" << endl;
        asio::async_write(serialPort, asio::buffer(write_msgs_.front().get()->data(), write_msgs_.front().get()->size()), [&](const asio::error_code& error, size_t bytes_transferred)
        {
            //cout << "Wrote : " << bytes_transferred << "bytes" << endl;
            if (!error)
            {
                write_msgs_.pop_front();
                if (!write_msgs_.empty())
                {
                    //std::cout << "Finished writing." << std::endl;
                }
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
    atomic<bool> active_;
    atomic<bool> transferring_;
    mutex transfer_mutex;
    condition_variable transfer_waiting;

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
    payload->push_back(location & 0xff);
    payload->push_back(location >> 8);
    payload->push_back(length & 0xff);
    payload->push_back(length >> 8);
    return payload;
}


shared_ptr<vector<uint8_t>> send_string(string string_to_load, int spectrumAddress)
{
    shared_ptr<vector<uint8_t>> payload = make_payload(0xaa, spectrumAddress, static_cast<uint16_t>(string_to_load.size() + 1));
    payload->insert(std::end(*payload), std::begin(string_to_load), std::end(string_to_load));
    payload->push_back(0);
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
    const int transferAmount = static_cast<int>(fileData.size());
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

    // Get the value at (SP) and decrement SP twice, this is so we can send SP as register data to the spectrum and have a uniform executor.
    // .SNA files store the PC at the current SP.
    uint16_t *pStackPointer =  reinterpret_cast<uint16_t*>(snapshotData.data() + 23);
    uint16_t programCounter = *reinterpret_cast<uint16_t*>(snapshotData.data() + 27 + *pStackPointer - 0x4000);
    *pStackPointer += 2;

    // register details payload
    payload = make_payload(0xa0, 0, 29);
    payload->insert(std::end(*payload), std::begin(snapshotData) + snapshotIndex, std::begin(snapshotData) + snapshotIndex + 27);
    payload->emplace_back(programCounter & 0xff);
    payload->emplace_back(programCounter >> 8);
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

void decompress_z80_block(uint16_t spectrumAddress, uint16_t dataSize, bool compressed, const std::vector<char>::iterator dataBegin, const std::vector<char>::iterator dataEnd, vector<shared_ptr<vector<uint8_t>>> &payloads)
{
    shared_ptr<vector<uint8_t>> payload;
    std::vector<uint8_t> decompressedData(dataSize);
    auto inData = dataBegin;
    auto outData = decompressedData.begin();
    if (compressed)
    {
        while (inData != dataEnd && outData != decompressedData.end())
        {
            uint8_t val = *inData++;
            if (val == 0xed && inData != dataEnd && static_cast<uint8_t>(*inData) == 0xed)
            {
                ++inData;
                uint8_t cnt = *inData++;
                val = *inData++;
                while (cnt-- > 0 && outData != decompressedData.end())
                {
                    *outData++ = val;
                }
            }
            else
            {
                *outData++ = val;
            }
        }
    }
    else
    {
        while (inData != dataEnd && outData != decompressedData.end())
        {
            *outData++ = static_cast<uint8_t>(*inData++);
        }
    }

    const int blocksize = 9000;
    std::vector<uint8_t>::iterator inputData = decompressedData.begin();
    for (int block = 0; block < dataSize / blocksize; ++block)
    {
        payload = make_payload(0xaa, spectrumAddress, blocksize);
        payload->insert(std::end(*payload), inputData, inputData + blocksize);
        payloads.push_back(payload);
        spectrumAddress += blocksize;
        inputData += blocksize;
    }
    if (dataSize % blocksize)
    {
        payload = make_payload(0xaa, spectrumAddress, dataSize % blocksize);
        payload->insert(std::end(*payload), inputData, inputData + (dataSize % blocksize));
        payloads.push_back(payload);
    }
}

vector<shared_ptr<vector<uint8_t>>> send_z80(string snapshot_to_load)
{
    vector<shared_ptr<vector<uint8_t>>> payloads;

    ifstream snapshotFile(snapshot_to_load, ios::binary | ios::ate);
    vector<char> snapshotData(snapshotFile.tellg());
    snapshotFile.seekg(0, ios::beg);
    snapshotFile.read(snapshotData.data(), snapshotData.size());


    Z80_V1_Shared_Regs *inRegs = reinterpret_cast<Z80_V1_Shared_Regs *>(snapshotData.data());
    const bool isV1 = inRegs->pc != 0;

    SmartLINK_Regs outRegs;
    outRegs.i = inRegs->i;
    outRegs._hl = inRegs->_hl;
    outRegs._de = inRegs->_de;
    outRegs._bc = inRegs->_bc;
    outRegs._af = inRegs->_f | (inRegs->_a << 8);
    outRegs.hl = inRegs->hl;
    outRegs.de = inRegs->de;
    outRegs.bc = inRegs->bc;
    outRegs.iy = inRegs->iy;
    outRegs.ix = inRegs->ix;
    outRegs.ei = inRegs->ei;
    outRegs.r = inRegs->r;
    outRegs.af = inRegs->f | (inRegs->a << 8);
    outRegs.sp = inRegs->sp;
    outRegs.im = inRegs->im;
    outRegs.pc = isV1 ? inRegs->pc : static_cast<Z80_V2_Shared_Regs*>(inRegs)->_pc; // v2 has pc address for all subsequent versions.
    outRegs.border = inRegs->border;


    int snapshotIndex = sizeof(Z80_V1_Shared_Regs) + (isV1 ? 0 : (static_cast<Z80_V2_Shared_Regs*>(inRegs)->headerSize + sizeof(uint16_t))); // v2 has extra header size for all subsequent versions.
    shared_ptr<vector<uint8_t>> payload;

    // register details payload
    auto const ptr = reinterpret_cast<uint8_t*>(&outRegs);
    std::vector<uint8_t> regsAsBuffer(ptr, ptr + sizeof(outRegs));
    payload = make_payload(0xa0, 0, static_cast<uint16_t>(regsAsBuffer.size()));
    payload->insert(std::end(*payload), regsAsBuffer.begin(), regsAsBuffer.end());
    payloads.push_back(payload);

    if (isV1)
    {
        decompress_z80_block(0x4000, 48 * 1024, true, snapshotData.begin() + snapshotIndex, snapshotData.end(), payloads);
    }
    else
    {
        Z80_V2_Shared_Regs *inRegsV2 = static_cast<Z80_V2_Shared_Regs*>(inRegs);
        std::vector<char>::iterator data = snapshotData.begin() + snapshotIndex;
        while (data < snapshotData.end())
        {
            bool chunkCompressed = true;
            Z80_Page_Chunk_Header *dataChunkHeader = reinterpret_cast<Z80_Page_Chunk_Header *>(&(*data));
            if (dataChunkHeader->compressedDataSize == 0xffff)
            {
                chunkCompressed = false;
                dataChunkHeader->compressedDataSize = 16384;
            }
            std::vector<char>::iterator compressedData = data + offsetof(Z80_Page_Chunk_Header, data);
            if (inRegsV2->hardwareMode == 0)
            {
                switch (dataChunkHeader->pageNumber)
                {
                case 4:
                    decompress_z80_block(0x8000, 16384, chunkCompressed, compressedData, compressedData + dataChunkHeader->compressedDataSize, payloads);
                    break;
                case 5:
                    decompress_z80_block(0xC000, 16384, chunkCompressed, compressedData, compressedData + dataChunkHeader->compressedDataSize, payloads);
                    break;
                case 8:
                    decompress_z80_block(0x4000, 16384, chunkCompressed, compressedData, compressedData + dataChunkHeader->compressedDataSize, payloads);
                    break;
                default:
                    break;
                }
            }
            else if (inRegsV2->hardwareMode == 3)
            {

            }
            data += offsetof(Z80_Page_Chunk_Header, data) + dataChunkHeader->compressedDataSize;
        }
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
    payload->push_back(code & 0xff);
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
                c.write(payloads);
            }
            if (s == "z80")
            {
                string snap_to_load;
                cin >> snap_to_load;
                vector<shared_ptr<vector<uint8_t>>> payloads = send_z80(snap_to_load);
                c.write(payloads);
            }
            if (s == "reset")
            {
                c.write(make_smartlink_action(1));
            }
            if(s=="port")
            {
                int address;
                uint8_t value;
                cin >> address >> value;
                shared_ptr<vector<uint8_t>> payload;
                payload = make_payload(0xbb, 0, 3);
                payload->push_back(address & 0xff);
                payload->push_back(address >> 8);
                payload->push_back(value);
                c.write(payload);
            }
            if (s == "data")
            {
                string file_to_load;
                int address;
                cin >> file_to_load >> address;
                vector<shared_ptr<vector<uint8_t>>> payloads = send_file(file_to_load, address);
                c.write(payloads);
            }
            if (s == "string")
            {
                string string_to_load;
                int address;
                cin >> string_to_load >> address;
                c.write(send_string(string_to_load, address));
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
