#include "fs.h"
#include <iostream>
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

int FS::find_free_block() {
    cout << "[P1] find_free_block stub\n";
    return -1;
}

vector<uint16_t> FS::get_chain(uint16_t start_blk) {
    cout << "[P1] get_chain stub\n";
    return { start_blk };
}

void FS::free_chain(uint16_t start_blk) {
    cout << "[P1] free_chain stub\n";
}

void FS::load_fat() {
    cout << "[P1] load_fat stub\n";
}

void FS::save_fat() {
    cout << "[P1] save_fat stub\n";
}


// formats the disk, i.e., creates an empty file system
int
FS::format()
{
    std::cout << "FS::format()\n";
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
