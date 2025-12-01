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
        cerr << e.what() << endl;
        cout << "Formatting...\n";
        format();
    }

    cwd_block = ROOT_BLOCK;
    cwd_path = "/";

    std::cout << "FS::FS()... Creating file system\n";
}

FS::~FS()
{
    // Person 1: ensure FAT is saved before exit
    try {
        save_fat();
    } catch (const std::runtime_error& e){
        cerr << e.what() << endl;
    }
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
    // cout << "FAT saved to disk\n";
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

    /*
    Thanh: Du har kollat uppe att om entrie_size > DIR_ENTRIES => entrie_size = DIR_ENTRIES
    man inte ändrat det neren
    => använda entrie_size i for-loop istället för entries.size()
    
    dir_entry* p = reinterpret_cast<dir_entry*>(buffer);
    for (int i = 0; i < entrie_size; i++) {
        p[i] = entries[i];
    }
    */
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

int FS::mkdir(string dirpath) {
    if (dirpath.empty() || dirpath.size() > 55) {
        cout << "name not valid" << endl;
        return -1;
    }

    dir_entry parent_entry{};
    string new_name;
    if (!resolve_path(dirpath, parent_entry, new_name)) {
        cout << "no such directory (parent) for: " << dirpath << endl;
        return -1;
    }
    uint16_t parent_block = parent_entry.first_blk;

    vector<dir_entry> entries; // array att fylla
    int res = load_dir(parent_block, entries);
    if (res < 0) {
        std::cout << "could not load current directory\n";
        return -1;
    }

    for (size_t i = 0; i < entries.size(); i++) {
        if (new_name == entries[i].file_name) {
            cout << "Directory already exists" << endl;
            return -1;
        }
    }

    if (entries.size() >= DIR_ENTRIES) {
        cout << "Directory full" << endl;
        return -1;
    }

    int new_block = alloc_block();
    if (!new_block) {
        cout << "disk is full" << endl;
        return -1;
    }

    /*
    Thanh: alloc_block() ska returnerar blk om det finns och kasta
    ett fel medđelandet om disken är ful så bäst att man lägga
    new_block=alloc_block() i ett try-catch

    int new_block;
    try {
        new_block = alloc_block();
    } catch (const std::runtime_error& e) {
        cout << "disk is full\n";
        return -1;
    }
    */

    dir_entry map{};
    strncpy(map.file_name, new_name.c_str(), sizeof(map.file_name) - 1);
    map.size = 0;
    map.first_blk = new_block; // index in the FAT for the first block of the file
    map.type = TYPE_DIR; // directory (1) or file (0)
    map.access_rights = READ | WRITE | EXECUTE;  // read (0x04), write (0x02), execute (0x01)
    entries.insert(entries.begin(), map);

    res = save_dir(parent_block, entries);
    if (res < 0) {
        cout << "could not save current directory" << endl;
        return -1;
    }

    save_fat();
    return 0;
}

// int
// FS::mkdir(std::string dirpath)
// {
//     std::cout << "FS::mkdir(" << dirpath << ")\n";
//     return 0;
// }

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls() {
    vector<dir_entry> entries;
    //ta ut hur många entires det fins

    int status_block = load_dir(cwd_block, entries); //leta upp
    if (status_block != 0) {
        cout << "ls: could not load directory block" << endl;
        return -1;
    }

    cout << "name" << "\t" << "type"  << "\t" <<"size" << endl;
    for (int i = 0; i < entries.size() ; i++) {
        string type = "";
        if(entries[i].type == TYPE_DIR){
            type = "dir";
        }else{
            type = "file";
        }
        string size;
        if(entries[i].size == 0){
            size = "-";
        }else{
            size = to_string(entries[i].size);
        }
        if (entries[i].file_name[0] != '\0') {
            cout << entries[i].file_name << "\t" << type << "\t" << size;
        }
        cout << "\n";
    }
    return 0;
}


// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
// int
// FS::cd(std::string dirpath)
// {
//     std::cout << "FS::cd(" << dirpath << ")\n";
//     return 0;
// }

int FS::cd(string name) {
    dir_entry parent;
    string last_name;
    string path = name;

    if (!resolve_path(path, parent, last_name, true)) {
        cout << "directory not found" << endl;
        return -1;
    }
    if (parent.first_blk == 0){
        cwd_block = parent.first_blk;
        cwd_path = path;
        return 0;
    }

    if (parent.type == TYPE_FILE){
        cout << "can not do cd on a file" << endl;
        return -1;
    }
    cwd_block = parent.first_blk;
    cwd_path = path;

    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    std::cout << cwd_path << endl;
    return 0;
}

bool FS::resolve_path(string& path_in, dir_entry& entry, string& new_name, bool throw_not_found) {
    string path = path_in;
    if(path.substr(0, 1) != "/"){
        if(cwd_path.substr(cwd_path.length()-1, 1) != "/"){
            path = cwd_path + "/" + path;
        }
        else{
            path = cwd_path + path;
        }
    }

    string current;
    vector<std::string> parts;
    for (int i = 0; i < path.size(); ++i) {
        char path_list = path[i];

        if (path_list == '/') {
            if (!current.empty()) {
                if(current == ".."){
                    parts.pop_back();
                }
                parts.push_back(current);
                current.clear();
            }
        }
        else
        {
            current.push_back(path_list);
        }
    }
    if (!current.empty()) {
        if(current == ".."){
            parts.pop_back();
        }else{
            parts.push_back(current);
        }
    }

    path_in = "";
    for(auto i: parts){
        path_in += "/" + i;
    }
    if(parts.size() != 0){
        new_name = parts.back();
    }else{
        path_in = "/";
    }

    uint16_t current_block = ROOT_BLOCK;
    entry = dir_entry{};
    entry.first_blk = current_block;
    for(auto name: parts){
        if (!parts.empty()){
            vector<dir_entry> entires;
            if (load_dir(current_block, entires) < 0) {
                return false;
            }

            bool found_map = false;

            for(int i = 0; i < entires.size(); i++){
                if(entires[i].type == TYPE_DIR && entires[i].file_name == name){
                    entry = entires[i];
                    current_block = entires[i].first_blk;
                    found_map = true;
                    continue;
                }
            }
            if (throw_not_found == true && found_map == false){
                return false;
            }
        }
    }
    return true;
}


/* ============================================================
   ====================== PERSON 3 =============================
   ===================== FILE OPERATIONS ======================
   ============================================================ */

//create <filepath> creates a new file on the disk, the data content is
//written on the following rows (ended with an empty row)
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



/* För test 1 och test 3
int FS::create(string filename)
{
    if (filename.size() > 55) {
    cout << "Filename too long\n";
    return -1;
    }
    vector<dir_entry> dir;

    load_dir(cwd_block, dir);
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
    entry.type = TYPE_FILE;
    entry.access_rights = READ|WRITE;
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
    save_dir(cwd_block, dir);

    cout << "File created.\n";
    return 0;
}

//Thanh
int 
FS::cat(std::string filepath) {
    vector<dir_entry> dir;
    if (load_dir(cwd_block, dir) != 0) {
        cout << "could not load current directory\n";
        return -1;
    }

    dir_entry file = {};
    bool found = false;
    for (auto& e : dir) {
        if (filepath == e.file_name) {
            if(e.type == TYPE_DIR){
                cerr << "Try to use cat with directory";
                return -1;
            } else {
                file = e; found = true;
                break; 
            } 
        }
    }
    if (!found) { cout << "File not found\n"; return -1; }

    int remaining = file.size;
    int16_t blk = file.first_blk;
    if (blk == FAT_EOF && remaining > 0) {
        cout << "Corrupted file (no blocks)\n";
        return -1;
    }

    while (remaining > 0) {
        uint8_t buffer[BLOCK_SIZE] = {0};
        if (disk.read(blk, buffer) != 0) {
            cout << "disk read error\n"; return -1;
        }
        int to_print = min(remaining, BLOCK_SIZE);
        cout << string((char*)buffer, to_print);
        remaining -= to_print;
        if (remaining > 0) blk = fat[blk];
    }
    return 0;
}

*/