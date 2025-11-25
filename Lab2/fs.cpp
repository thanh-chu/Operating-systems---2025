#include "fs.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>

using namespace std;


FS::FS()
{
    // Person 1: load FAT + root directory
    load_fat();
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
    for(int16_t i = 2; i < static_cast<int16_t>(BLOCK_SIZE/2); i++){ //formatting the disk, i.e., initializing the FAT and marking all blocks as free (except block 0 (the root directory) and block 1 (the FAT))
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
    if(blk < 0){
        return -1;
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
    for(int16_t i = 0; i < BLOCK_SIZE/2; i++){
        fat[i] = FAT_FREE;
    }
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;

    save_fat();

    uint8_t empty_block[BLOCK_SIZE] = {0};
    disk.write(ROOT_BLOCK, empty_block);

    cout << "Format completed\n";
    return 0;
}

/* ============================================================
   ====================== PERSON 2 =============================
   ================= DIRECTORY MANAGEMENT ======================
   ============================================================ */

bool FS::load_dir(uint16_t blk, vector<dir_entry>& list) {
    cout << "[P2] load_dir stub\n";
    list.clear();
    return true;
}

bool FS::save_dir(uint16_t blk, const vector<dir_entry>& list) {
    cout << "[P2] save_dir stub\n";
    return true;
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
int
FS::ls()
{
    std::cout << "FS::ls()\n";
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
int
FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";
    return 0;
}

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
