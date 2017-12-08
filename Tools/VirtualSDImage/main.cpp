//
//  VirtualSDImage for SmartLINK ROM by Paul Tankard
//
//  http://www.tavi.co.uk/phobos/fat.html
//
//  g++ -std=c++11 main.cpp -o smartsd
//  clang++ -std=c++11 main.cpp -o smartsd
//
// TODO:
//   error checking
//   docs
//

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

#ifdef _MSC_VER
#pragma pack(push)
#pragma pack(1)
#endif
struct MBR {
	uint8_t bootstrap_jmp[3];
	uint8_t manufacturer[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t	num_fat_tables;
	uint16_t root_dir_entries;
	uint16_t total_sectors_on_disk_16;
	uint8_t media_desc;
	uint16_t sectors_per_fat;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t num_hidden_sectors;
	uint32_t total_sectors_on_disk_32;
	uint16_t physical_drive_number;
	uint8_t ex_boot_record;
	uint32_t serial_number;
	uint8_t volume_label[11];
	uint8_t file_system_id[8];
}
#ifdef __GNUC__
__attribute((packed))
#endif
_mbr {
	{ 0, 0, 0 },									// (3)   Part of the bootstrap program. (x86)
	{ 'H','E','X','T','A','N','K','.' },			// (8)   Optional manufacturer description.
	512, 											// (2)   Number of bytes per sector (almost always 512).
	32, 											// (1)   Number of sectors per cluster.
	1, 												// (2)   Number of reserved sectors. This is the number of sectors on the disk that are not actually part of the file system; in most cases this is exactly 1, being the allowance for the boot block.
	2, 												// (1)   Number of File Allocation Tables.
	16,	 											// (2)   Number of root directory entries (including unused ones).
	0, 												// (2)   Total number of sectors in the entire disk. If the disk size is larger than 65535 sectors (and thus will not fit in these two bytes), this value is set to zero, and the true size is stored at offset 0x20.
	0xF0, 											// (1)   Media Descriptor. This is rarely used, but still exists. .
	1, 												// (2)   The number of sectors occupied by one copy of the File Allocation Table.
	0, 												// (2)   The number of sectors per track. This information is present primarily for the use of the bootstrap program, and need not concern us further here.
	0, 												// (2)   The number of heads (disk surfaces). This information is present primarily for the use of the bootstrap program, and need not concern us further here.
	0, 												// (4)   The number of hidden sectors. The use of this is largely historical, and it is nearly always set to 0; thus it can be ignored.
	0,			 									// (4)   Total number of blocks in the entire disk (see also offset 0x13).
	0x0000, 										// (2)   Physical drive number. This information is present primarily for the use of the bootstrap program, and need not concern us further here.
	0x00, 											// (1)   Extended Boot Record Signature This information is present primarily for the use of the bootstrap program, and need not concern us further here.
	0, 												// (4)   Volume Serial Number. Unique number used for identification of a particular disk.
	{ 'S','M','A','R','T','L','I','N','K',' ',' ' },// (11)  Volume Label. This is a string of characters for human-readable identification of the disk (padded with spaces if shorter); it is selected when the disk is formatted.
	{ 'F','A','T','1','6',' ',' ',' ' },			// (8)   File system identifier (padded at the end with spaces if shorter).
};

struct Fat16File {
	uint8_t filename[8];
	uint8_t ext[3];
	uint8_t attributes;
	uint8_t reserved[10];
	uint16_t modify_time;
	uint16_t modify_date;
	uint16_t starting_cluster;
	uint32_t file_size;
}
#ifdef __GNUC__
__attribute((packed))
#endif
_files[] = {
	{
		// Disk label.
		{ 'S','M','A','R','T','L','I','N' },		// (8)   Filename.
		{ 'K',' ',' ' },							// (3)   Filename extension.
		0x28,										// (1)   File attributes. (archive|volume_label)	 - probably don't need archive flag, only needed for backup, I think.
		{},											// (10)  Reserved.
		0,											// (2)   Time created or last updated.
		0,											// (2)   Date created or last updated.
		0,											// (2)   Starting cluster number for file. (first two are reserved)
		0											// (4)   File size in bytes.
	},
	{
		// ROM file.
		{ 'S','M','A','R','T','L','N','K' },		// (8)   Filename.
		{ 'R','O','M' },							// (3)   Filename extension.
		0x21,										// (1)   File attributes. (archive|read_only)		- probably don't need archive flag, only needed for backup, I think.
		{},											// (10)  Reserved.
		0,											// (2)   Time created or last updated.
		0,											// (2)   Date created or last updated.
		2,											// (2)   Starting cluster number for file. (first two are reserved)
		16384										// (4)   File size in bytes.
	}
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

int main(int argc, const char * argv[]) {
	
	std::vector<uint8_t> sdcard((4 * 512) + (16 * 1024));
	
	// Master boot record.
	memcpy(sdcard.data(), &_mbr, sizeof(_mbr));
	
	// Boot block signature.
	sdcard[510] = 0x55;
	sdcard[511] = 0xAA;
	
	// Fat 1 table.
	sdcard[512] = 0xf0;
	sdcard[513] = 0xff;
	sdcard[514] = 0xff;
	sdcard[515] = 0xff;
	sdcard[516] = 0xff;		// This is the next cluster for the file, given our cluster is 16K (BytesPerSector(512) * SectorsPerCluster(32))
	sdcard[517] = 0xff;		// Means next cluster if file size is bigger than one cluster, 0xffff means END.
	
	// Fat 2 table.
	sdcard[1024] = 0xf0;
	sdcard[1025] = 0xff;
	sdcard[1026] = 0xff;
	sdcard[1027] = 0xff;
	sdcard[1028] = 0xff;	// This is the next cluster for the file, given our cluster is 16K (BytesPerSector(512) * SectorsPerCluster(32))
	sdcard[1029] = 0xff;	// Means next cluster if file size is bigger than one cluster, 0xffff means END.
	
	// Root directory.
	memcpy(sdcard.data() + 1536, _files, sizeof(_files));
	
	// Read ROM into first cluster.
	std::ifstream romFile(argv[1], std::ios::binary);
	std::copy(std::istreambuf_iterator<char>(romFile), std::istreambuf_iterator<char>(), sdcard.begin() + 2048);
	
	// Save out bin2c.
	std::ofstream output(argv[2], std::ios::trunc);
	output << "uint8_t virtual_smartlink_sdcard[] = {";
	for (std::vector<uint8_t>::const_iterator it = sdcard.cbegin(); it != sdcard.cend(); ++it)
	{
		if (!((it - sdcard.cbegin()) % 16))
		{
			output << "\n\t";
		}
		
		output << "0x" << "0123456789ABCDEF"[*it >> 4] << "0123456789ABCDEF"[*it & 15];
		
		if (it != sdcard.cend() - 1)
		{
			output << ",";
		}
	}
	output << "\n};";
	
	return 0;
}

