#include <iostream>
#include <cstdint>
#include "disk.h"

#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0 //root directory
#define FAT_BLOCK 1 //block save FAT on disk
#define FAT_FREE 0
#define FAT_EOF -1

#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01
#define DIR_ENTRIES (BLOCK_SIZE / sizeof(dir_entry))


using namespace std;

struct dir_entry {
    char file_name[56]; // name of the file / sub-directory
    uint32_t size; // size of the file in bytes
    uint16_t first_blk; // index in the FAT for the first block of the file
    uint8_t type; // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)
};

class FS {
private:
    Disk disk;
    // size of a FAT entry is 2 bytes
    int16_t fat[BLOCK_SIZE/2];

    uint16_t cwd_block = ROOT_BLOCK;     // --- added for person 2 --- nuvarande block
    std::string cwd_path = "/";          // --- added for person 2 --- nuvarande path

public:
    FS();
    ~FS();

/* =====================================================
       =============== PERSON 1: FAT MANAGER ================
       ===================================================== */

    int find_free_block();                          // --- added ---
    int alloc_block();                              // --- added ---
    vector<uint16_t> get_chain(uint16_t first_blk); // --- added ---
    void free_chain(uint16_t first_blk);            // --- added ---
    void load_fat();                                // --- added ---
    void save_fat();                                // --- added ---
    // formats the disk, i.e., creates an empty file system
    int format();
   

    /* =====================================================
       =========== PERSON 2: DIRECTORY MANAGEMENT ===========
       ===================================================== */

    bool load_dir(uint16_t blk, std::vector<dir_entry>& list);    // --- added ---
    bool save_dir(uint16_t blk, const std::vector<dir_entry>& list); // --- added ---
    // mkdir <dirpath> creates a new sub-directory with the name <dirpath>
    // in the current directory
    int mkdir(std::string dirpath);
     // ls lists the content in the current directory (files and sub-directories)
    int ls();
    // cd <dirpath> changes the current (working) directory to the directory named <dirpath>
    int cd(std::string dirpath);
    // pwd prints the full path, i.e., from the root directory, to the current
    // directory, including the current directory name
    int pwd();
    bool resolve_path(std::string path, uint16_t& blk, dir_entry& entry); // --- added ---


    /* =====================================================
       =========== PERSON 3: FILE MANAGEMENT ==========
       ===================================================== */

    // create <filepath> creates a new file on the disk, the data content is
    // written on the following rows (ended with an empty row)
    int create(std::string filepath);
    // cat <filepath> reads the content of a file and prints it on the screen
    int cat(std::string filepath);
    // cp <sourcepath> <destpath> makes an exact copy of the file
    // <sourcepath> to a new file <destpath>
    int cp(std::string sourcepath, std::string destpath);
    // mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
    // or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
    int mv(std::string sourcepath, std::string destpath);
    // rm <filepath> removes / deletes the file <filepath>
    int rm(std::string filepath);
    // append <filepath1> <filepath2> appends the contents of file <filepath1> to
    // the end of file <filepath2>. The file <filepath1> is unchanged.
    int append(std::string filepath1, std::string filepath2);
    // chmod <accessrights> <filepath> changes the access rights for the
    // file <filepath> to <accessrights>.
    int chmod(std::string accessrights, std::string filepath);
};

#endif // __FS_H__

/*
Just för test task1:
vector<dir_entry> FS::read_dir()
{
    uint8_t buffer[BLOCK_SIZE];
    vector<dir_entry> entries;

    if (disk.read(ROOT_BLOCK, buffer) < 0)
        throw runtime_error("Cannot read root directory");

    dir_entry* p = (dir_entry*) buffer;

    for (int i = 0; i < DIR_ENTRIES; i++) {
        if (p[i].file_name[0] != '\0') {
            entries.push_back(p[i]);
        }
    }

    return entries;
}

void FS::write_dir(const vector<dir_entry>& entries)
{
    uint8_t buffer[BLOCK_SIZE] = {0};
    dir_entry* p = (dir_entry*) buffer;

    for (int i = 0; i < entries.size(); i++) {
        p[i] = entries[i];
    }

    disk.write(ROOT_BLOCK, buffer);
}

int FS::create(string filename)
{
    if (filename.size() > 55) {
    cout << "Filename too long\n";
    return -1;
    }
    vector<dir_entry> dir = read_dir();
    if (dir.size() >= DIR_ENTRIES) {
    cout << "Root directory full\n";
    return -1;
    }
    // Check duplicate name
    for (auto& e : dir) {
        if (filename == e.file_name) {
            cout << "File already exists\n";
            return -1;
        }
    }

    // Create directory entry
    dir_entry entry = {};
    strncpy(entry.file_name, filename.c_str(), sizeof(entry.file_name));
    entry.size = 0;
    entry.first_blk = FAT_EOF;

    cout << "Enter file contents (empty line stops):\n";

    string line;
    vector<uint8_t> content;

    //cin.ignore();
    while (true) {
        getline(cin, line);
        if (line.empty()) break;
        for (char c : line) content.push_back(c);
        content.push_back('\n');
    }

    entry.size = content.size();

    // Allocate blocks
    if (content.size() > 0) {
        int16_t first = alloc_block();
        if (first < 0) {
            cout << "Disk full\n";
            return -1;
        }
        entry.first_blk = first;

        int16_t curr = first;

        size_t pos = 0;

        while (pos < content.size()) {
            uint8_t buffer[BLOCK_SIZE] = {0};

            size_t bytes = min((size_t)BLOCK_SIZE, content.size() - pos);
            memcpy(buffer, &content[pos], bytes);

            disk.write(curr, buffer);

            pos += bytes;

            if (pos < content.size()) {
                int16_t next = alloc_block();
                fat[curr] = next;
                curr = next;
            }
        }
    }

    // Save FAT
    save_fat();

    // Add to directory
    dir.push_back(entry);
    write_dir(dir);

    cout << "File created.\n";
    return 0;
}

int FS::ls()
{
    vector<dir_entry> dir = read_dir();

    cout << "name   size\n";
    for (auto& e : dir) {
        cout << e.file_name << "   " << e.size << "\n";
    }

    return 0;
}

int FS::cat(string filename)
{
    vector<dir_entry> dir = read_dir();

    dir_entry file = {};
    bool found = false;

    for (auto& e : dir) {
        if (filename == e.file_name) {
            file = e;
            found = true;
            break;
        }
    }

    if (!found) {
        cout << "File not found\n";
        return -1;
    }

    int remaining = file.size;
    int16_t blk = file.first_blk;

    while (remaining > 0) {
        uint8_t buffer[BLOCK_SIZE];

        disk.read(blk, buffer);

        int to_print = min(remaining, BLOCK_SIZE);

        cout << string((char*)buffer, to_print);

        remaining -= to_print;

        if (remaining > 0)
            blk = fat[blk];
    }

    return 0;
}

*/
