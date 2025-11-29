#include "fs.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <cstring>

using namespace std;


FS::FS()
{
    // Person 1: load FAT + root directory
    try {
        load_fat();
    } catch (const std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        std::cout << "Formatting...\n";
        format();
    }

    cwd_block = ROOT_BLOCK;
    cwd_path = "/";

    std::cout << "FS::FS()... Creating file system\n";
}

FS::~FS()
{
    // Person 1: ensure FAT is saved before exit
    save_fat();
}

/* ============================================================
   ====================== PERSON 1 =============================
   =================== FAT + DISK MANAGER ======================
   ============================================================ */

//check all FAT and return the first free block from the first block, help-funtion for get_chain
int
FS::find_free_block() {
    for(int i = 2; i < (BLOCK_SIZE/2); i++){ //formatting the disk, i.e., initializing the FAT and marking all blocks as free (except block 0 (the root directory) and block 1 (the FAT))
        if(fat[i] == FAT_FREE){
            return i;
        }
    }
    return -1;
}

//allocate a new block data from FAT => use in create file/directory, append, cp, mv file
int
FS::alloc_block() {
    int blk = find_free_block();
    if (blk == -1){
        throw runtime_error("Error: Disk full, can not allocate new block.\n");
    }
    fat[blk] = FAT_EOF;
    return blk ;
}

//return a vector with all blocks of file/directory (input is the first block)
vector<uint16_t>
FS::get_chain(uint16_t first_blk) {
    vector<uint16_t> chain;
    while(true){
        if(first_blk >= BLOCK_SIZE/2){
            throw runtime_error("Corrupted FAT: index out of range");
        }
        if(fat[first_blk] == FAT_FREE){
            throw runtime_error("Corrupted FAT: encounted free block inside file chain");
        }
        if(first_blk == ROOT_BLOCK || first_blk == FAT_BLOCK){
            throw runtime_error("Corrupted FAT: file chain points to reserved block");
        }
        chain.push_back(first_blk);
        int16_t next = fat[first_blk];
        if(next == FAT_EOF){
            break;
        }
        first_blk = static_cast<uint16_t>(next);
    }
    return chain;
}

//free all blocks of file/directory, start with first_blk
void
FS::free_chain(uint16_t first_blk) {
    while(true){
        if(first_blk == ROOT_BLOCK || first_blk == FAT_BLOCK){
            throw runtime_error("Error: try to free reserved block!");
        }
        int16_t next = fat[first_blk];
        fat[first_blk] = FAT_FREE;

        if(next == FAT_EOF){
            break;
        }
        if(next < 0 || next >= BLOCK_SIZE/2){
            throw runtime_error("Corrupted FAT: invalid FAT pointer!");
        }
        first_blk = static_cast<uint16_t>(next);
    }
}

//load filesystem allocated table from disk to RAM (mount filesystem)
void
FS::load_fat() {
    uint8_t buffer[BLOCK_SIZE];
    if(disk.read(FAT_BLOCK, buffer) == -1){
        throw runtime_error("Error: Cannot read FAT block");
    }

    memcpy(fat, buffer, sizeof(fat));
    cout << "FAT loaded into RAM\n";
}

//save FAT from RAM to disk to be sure that every changes on FAT will be saved (unmount filesystem)
void
FS::save_fat() {
    uint8_t buffer[BLOCK_SIZE];
    memcpy(buffer, fat, sizeof(fat));
    if(disk.write(FAT_BLOCK, buffer) == -1){
        throw runtime_error("Error: Can not write FAT block");
    }
    cout << "FAT saved to disk\n";
}

// formats the disk, i.e., creates an empty file system
int
FS::format()
{
    uint8_t empty_block[BLOCK_SIZE] = {0};
    for(int i = 0; i < BLOCK_SIZE/2; i++){
        if(i == ROOT_BLOCK || i == FAT_BLOCK){
            fat[i] = FAT_EOF;
        } else {
            fat[i] = FAT_FREE;
        }
    }
    save_fat();
    disk.write(ROOT_BLOCK, empty_block);
    for(int i = 2; i < BLOCK_SIZE/2; i++){
        disk.write(i, empty_block);
    }
    cout << "Format completed\n";
    return 0;
}

/* ============================================================
   ====================== PERSON 2 =============================
   ================= DIRECTORY MANAGEMENT ======================
   ============================================================ */

// bool FS::load_dir(uint16_t blk, vector<dir_entry>& list) {
//     cout << "[P2] load_dir stub\n";
//     list.clear();
//     return true;
// }
int FS::load_dir(uint16_t block_no, vector<dir_entry>& entries) {
    uint8_t buffer[BLOCK_SIZE];

    int res = disk.read(block_no, buffer);
    if (res != 0) {
        return res;
    }

    dir_entry* p = reinterpret_cast<dir_entry*>(buffer);

    for (int i = 0; i < DIR_ENTRIES; i++) {
        if (p[i].file_name[0] != '\0') {
            entries.push_back(p[i]);
        }
    }

    return 0;
}

// bool FS::save_dir(uint16_t blk, const vector<dir_entry>& list) {
//     cout << "[P2] save_dir stub\n";
//     return true;
// }

int FS::save_dir(uint16_t block_no, vector<dir_entry>& entries) {
    uint8_t buffer[BLOCK_SIZE] = { 0 };

    int entrie_size = entries.size();
    if (entrie_size > DIR_ENTRIES) {
        entrie_size = DIR_ENTRIES;
    }

    dir_entry* p = reinterpret_cast<dir_entry*>(buffer);
    for (int i = 0; i < entries.size(); i++) {
        p[i] = entries[i];
    }

    int res = disk.write(block_no, buffer);
    if (res != 0) {
        return res;
    }
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls() {
    uint16_t block_no = 0;

    vector<dir_entry> entries;
    //ta ut hur många entires det fins

    int status_block = load_dir(block_no, entries); //leta upp
    if (status_block != 0) {
        cout << "ls: could not load directory block" << endl;
        return -1;
    }

    cout << "name     size" << endl;
    for (int i = 0; i < entries.size() ; i++) {
        if (entries[i].file_name[0] != '\0') {
            cout << entries[i].file_name << "\t" << entries[i].size << endl;
        }
        cout << "\n";
    }
    return 0;
}


// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    std::cout << "FS::pwd()\n";
    return 0;
}

bool FS::resolve_path(string path, uint16_t& blk, dir_entry& entry) {
    cout << "[P2] resolve_path stub: " << path << endl;
    return false;
}


/* ============================================================
   ====================== PERSON 3 =============================
   ===================== FILE OPERATIONS ======================
   ============================================================ */

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
// int
// FS::create(std::string filepath)
// {
//     std::cout << "FS::create(" << filepath << ")\n";
//     return 0;
// }

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}



int FS::create(string filename)
{
    if (filename.size() > 55) {
    cout << "Filename too long\n";
    return -1;
    }
    uint16_t block_no = 0;
    vector<dir_entry> dir;

    load_dir(block_no, dir);
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
    save_dir(block_no, dir);

    cout << "File created.\n";
    return 0;
}


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