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
}

//save FAT from RAM to disk to be sure that every changes on FAT will be saved (unmount filesystem)
void
FS::save_fat() {
    uint8_t buffer[BLOCK_SIZE];
    memcpy(buffer, fat, sizeof(fat));
    if(disk.write(FAT_BLOCK, buffer) == -1){
        throw runtime_error("Error: Can not write FAT block");
    }
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
    return 0;
}


//Thanh: Creat a function to check the right the access a directory/a file
bool FS::check_rights(const dir_entry &check_object, uint8_t rights) {
    return (check_object.access_rights & rights) == rights;
}

/* ============================================================
   ====================== PERSON 2 =============================
   ================= DIRECTORY MANAGEMENT ======================
   ============================================================ */


int FS::load_dir(uint16_t block_no, vector<dir_entry>& entries) {
    uint8_t buffer[BLOCK_SIZE];

    int res = disk.read(block_no, buffer);
    if (res != 0) {
        return res;
    }

    dir_entry* p = reinterpret_cast<dir_entry*>(buffer);
    //Thanh, changed int to size_t
    for (size_t i = 0; i < DIR_ENTRIES; i++) {
        if (p[i].file_name[0] != '\0') {
            entries.push_back(p[i]);
        }
    }

    return 0;
}


int FS::save_dir(uint16_t block_no, vector<dir_entry>& entries) {
    uint8_t buffer[BLOCK_SIZE] = { 0 };

    size_t entrie_size = entries.size();
    if (entrie_size > DIR_ENTRIES) {
        entrie_size = DIR_ENTRIES;
    }

    //Thanh, change int to size_t
    dir_entry* p = reinterpret_cast<dir_entry*>(buffer);
    for (size_t i = 0; i < entrie_size; i++) {
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
FS::mkdir(string dirpath) {

    //Thanh: dirpath.size() can be longer than 55, only name of directory need to less than 55
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

   //Thanh: added to check the access rights on a directory: WRITE with the parent directory
    if (parent_entry.file_name[0] != '\0'){
        if (!check_rights(parent_entry, WRITE)) {
            cout << "Permission denied: cannot create file in directory " << parent_entry.file_name << endl;
            return -1;
        }
    }

    uint16_t parent_block = parent_entry.first_blk;

    vector<dir_entry> parent_entries; // array att fylla
    int res = load_dir(parent_block, parent_entries);
    if (res < 0) {
        std::cout << "could not load current directory\n";
        return -1;
    }

    for (size_t i = 0; i < parent_entries.size(); i++) {
        if (new_name == parent_entries[i].file_name) {
            cout << "Directory already exists" << endl;
            return -1;
        }
    }

    if (parent_entries.size() >= DIR_ENTRIES) {
        cout << "Directory full" << endl;
        return -1;
    }

    int new_block;
    try {
        new_block = alloc_block();
    } catch (const std::runtime_error& e) {
        cout << "disk is full\n";
        return -1;
    }

    dir_entry new_dir_entry{};
    strncpy(new_dir_entry.file_name, new_name.c_str(), sizeof(new_dir_entry.file_name) - 1);
    new_dir_entry.size = 0;
    new_dir_entry.first_blk = new_block;
    new_dir_entry.type = TYPE_DIR;
    new_dir_entry.access_rights = READ | WRITE | EXECUTE;

    parent_entries.push_back(new_dir_entry);

    res = save_dir(parent_block, parent_entries);
    if (res < 0) {
        cout << "could not save parent directory" << endl;
        return -1;
    }

    vector<dir_entry> new_entries;

    dir_entry back{};
    strncpy(back.file_name, "..", sizeof(back.file_name) - 1);
    back.size = parent_entry.size;
    back.first_blk = parent_entry.first_blk;
    back.type = parent_entry.type;
    back.access_rights = parent_entry.access_rights;

    new_entries.push_back(back);

    res = save_dir(new_block, new_entries);
    if (res < 0) {
        cout << "could not save new directory" << endl;
        return -1;
    }

    save_fat();
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls() {
    vector<dir_entry> entries;
    //ta ut hur många entires det fins

    int status_block = load_dir(cwd_block, entries); //leta upp
    if (status_block != 0) {
        cout << "ls: could not load directory block" << endl;
        return -1;
    }

    //Thanh: need the access rights: READ of cwd

    cout << "name" << "\t" << "type"  << "\t" << "accessrights"  << "\t" <<"size" << endl;
    //Thanh, change int to size_t
    for (size_t i = 0; i < entries.size() ; i++) {
        string type = "";
        if(entries[i].type == TYPE_DIR){
            type = "dir";
        }else{
            type = "file";
        }
        string access_string = "";
        if(entries[i].access_rights & READ){
            access_string += "r";
        } else {
            access_string += "-";
        }
        if(entries[i].access_rights & WRITE){
            access_string += "w";
        } else {
            access_string += "-";
        }
        if(entries[i].access_rights & EXECUTE){
            access_string += "x";
        } else {
            access_string += "-";
        }

        string size;
        if(entries[i].size == 0){
            size = "-";
        }else{
            size = to_string(entries[i].size);
        }
        if (entries[i].file_name[0] != '\0') {
            cout << entries[i].file_name << "\t" << type << "\t" << access_string << "\t\t"<< size;
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

int
FS::cd(string name) {
    dir_entry parent;
    string last_name;
    string path = name;

    if (!resolve_path(path, parent, last_name, true)) {
        cout << "directory not found" << endl;
        return -1;
    }

    //Thanh: need to add more to cover the case when dirname is ’..’ kan inte ha ett namn som är det här!
    //Thanh: need the access rights (EXCECUTE)
    //Thanh: need to edit because if the last in path is file => does not exist in parent so we can not compare parent.type to catch exception

    if (path == "/"){
        cwd_block = ROOT_BLOCK;
        cwd_path = path;
        return 0;
    }

    vector<dir_entry> entries;
    if (load_dir(parent.first_blk, entries) < 0) {
        cout << "could not load directory\n";
        return -1;
    }

    for (auto &e : entries) {
        if (e.file_name == last_name) {
            if (e.type == TYPE_FILE) {
                cout << "can not do cd on a file" << endl;
                return -1;
            }
            cwd_block = e.first_blk;
            cwd_path  = path;
            return 0;
        }
    }

    cout << "directory not found" << endl;
    return -1;
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
    //Thanh, changed int to size_t
    for (size_t i = 0; i < path.size(); ++i) {
        char path_list = path[i];

        if (path_list == '/') {
            if (!current.empty()) {
                if(current == ".."){
                    if(parts.size()){
                        parts.pop_back();
                    }
                }
            else{
                parts.push_back(current);
            }
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
            if(parts.size()){
                parts.pop_back();
            }
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
            //Thanh, changed int to size_t
            for(size_t i = 0; i < entires.size(); i++){
                //Thanh added: to check if the path has form file/file or file/directory - begin
                if(entires[i].file_name == name && entires[i].type == TYPE_FILE)
                {
                    if (name != parts.back()) {
                        if (throw_not_found) {
                            throw runtime_error("Not a directory");
                        }
                        return false;
                    }
                    found_map = true;
                    break;
                }
                //Thanh added: to check if the path has form file/file or file/directory - end
              if(entires[i].type == TYPE_DIR && entires[i].file_name == name && check_rights(entires[i], EXECUTE) == true){
                //Thanh edit-  begin - to check if the directory is the last => not add to entry, only become new_name
                if(name == parts.back()){
                    new_name = name;
                    found_map = true;
                    break;
                }
                entry = entires[i];
                current_block = entires[i].first_blk;
                found_map = true;
                continue;
                //thanh edit - end
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
// int
// FS::create(std::string filepath)
// {
//     std::cout << "FS::create(" << filepath << ")\n";
//     return 0;
// }

// cat <filepath> reads the content of a file and prints it on the screen
// int
// FS::cat(std::string filepath)
// {
//     std::cout << "FS::cat(" << filepath << ")\n";
//     return 0;
// }

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    //Thanh: the access rights: READ (source), WRITE (dest dir)
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    //Thanh: the access rights: WRITE

    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    //Thanh: the access rights: WRITE
    //Check if filepath exists.
    dir_entry parent_entry = {};
    string filename;

    resolve_path(filepath, parent_entry, filename, true);

    //load directory
    vector<dir_entry> entries;
    dir_entry to_remove;
    load_dir(parent_entry.first_blk, entries);
    bool found = false;
    
    //Hitta rätt entry
    int counter = 0;
    for(auto& entry: entries){
        if(entry.file_name==filename){
            to_remove = entry;
            found = true;
            break;
        }
        counter++;
    }
    if(found == false){
        cout << "Entry not found" << endl;
        return -1;
    }

    if(!(to_remove.access_rights & WRITE)){
        cout << "No access to remove" << endl;
        return -1;
    }
    //Om entry är dir måste den vara tom för att få ta bort.
    vector<dir_entry> to_remove_entries;
    if(to_remove.type == TYPE_DIR){
        load_dir(to_remove.first_blk, to_remove_entries);
        if(to_remove_entries.size()!=0){
            cout << "Can not remove directory if not empty" << endl;
            return -1;
        }
    }

    //Remove from fat.
    int16_t curr = to_remove.first_blk;
    int16_t next;
    while(fat[curr]!=FAT_EOF){
        next = fat[curr];
        fat[curr] = FAT_FREE;
        curr = next;
    }
    fat[curr] = FAT_FREE;

    //Ta bort entry från entries.
    entries.erase(entries.begin() + counter);
    //spara dir och fat
    save_dir(parent_entry.first_blk, entries);
    save_fat();
    
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    //Thanh: the access rights: READ (filepath1), WRITE (filepath2)
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    int accessrights_as_int;
    try{
        accessrights_as_int = stoi(accessrights);
    } catch(const std::invalid_argument &e) {
        std::cout << "Wrong format accessrights" << std::endl;
        return -1;
    }

    if(accessrights_as_int>7 || accessrights_as_int < 0){
        std::cout << "Wrong format accessrights" << std::endl;
        return -1;
    }

    dir_entry parent_entry{};
    string file_name;

    if (!resolve_path(filepath, parent_entry, file_name)) {
        cout << "File not found from path\n";
        return -1;
    }

    vector<dir_entry> dir;
    if (load_dir(parent_entry.first_blk, dir) != 0) {
        cout << "could not load current directory\n";
        return -1;
    }

    dir_entry file = {};
    bool found = false;
    for (auto& entry: dir) {
        if (file_name == entry.file_name) {
            file = entry; found = true;
            entry.access_rights = accessrights_as_int;
            break;
        }
    }
    if (!found) { cout << "File not found\n"; return -1; }

    save_dir(parent_entry.first_blk, dir);
    //std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}



int
FS::find_nr_of_free_blocks() {
    int nr_of_free_blocks = 0;
    for(int i = 2; i < (BLOCK_SIZE/2); i++){
        if(fat[i] == FAT_FREE){
            nr_of_free_blocks++;
        }
    }
    return nr_of_free_blocks;
}

int FS::create(std::string filepath)
{

    dir_entry parent_entry{};
    string filename;

    if (!resolve_path(filepath, parent_entry, filename)) {
        cout << "Invalid path: cannot resolve parent directory\n";
        return -1;
    }

  //Thanh confict: Jag tycker att man ban skriva så här för att undvika att parent_entry är tom för att man skapar en fil on root
if (parent_entry.file_name[0] != '\0'){
        if (!check_rights(parent_entry, WRITE)) {
            cout << "Insufficient access rights" << parent_entry.file_name << endl;
            return -1;
        }
    }

    //Kontrollera om filename för långt. I så fall error!
    if(filename.size() > 55){
        std::cout << "Filename too long" << std::endl;
        return -1;
    }

    //Kontrollera om filename finns. I så fall error!
    vector<dir_entry> entries;

    //Thanh confict: begin

    if(load_dir(parent_entry.first_blk, entries) != 0){
        cout << "Could not load directory";
        return -1;
    }

    if (entries.size() >= DIR_ENTRIES) {
        cout << "Directory full\n";
        return -1;
    }
    //Thanh confict: end: ändra uppe för att kontrollera om man inte kan ladda ner directory och root är full: load_dir(parent_entry.first_blk, entries);

    for (size_t i = 0; i < entries.size(); i++) {
        if (filename == entries[i].file_name) {
            cout << "File already exists" << endl;
            return -1;
        }
    }

    //Om alla kontroller ok; skriv data på kommande rader:
    std::string buf, result = {};
    while (getline(std::cin, buf)) //read std::cin into buf
    {
        if (buf.empty()) // if buf is empty break loop
            break;
        result += buf + "\n";
    }
    size_t size = result.length(); // we check how many bytes needed for the file
    size_t needed_blocks = size/BLOCK_SIZE + 1; // here we check how many blocks we need


    //Thanh conflict: size_t needed_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    dir_entry entry = {};
    strncpy(entry.file_name, filename.c_str(), sizeof(entry.file_name));
    entry.type = TYPE_FILE;
    entry.access_rights = READ|WRITE;
    entry.size = size;
    entry.first_blk = FAT_EOF;


    vector<int> allocated_blocks = {};
    if (!result.empty()) {
        if(needed_blocks > static_cast<size_t>(find_nr_of_free_blocks())){
            std::cout << "Disk is full" << std::endl;
            return -1;
        }
        for (size_t i = 0; i < needed_blocks; i++){
            try {
                int new_alloc_block = alloc_block();
                allocated_blocks.push_back(new_alloc_block);
            } catch (const std::runtime_error& e) {
                cout << "disk is full\n";
                return -1;
            }

        }
        entry.first_blk = allocated_blocks[0];

        int16_t curr = allocated_blocks[0];
        size_t pos = 0;

        for (size_t i = 0; i < needed_blocks; i++) {
            uint8_t buffer[BLOCK_SIZE] = {0};

            size_t bytes = min((size_t)BLOCK_SIZE, result.size() - pos);
            memcpy(buffer, &result[pos], bytes);
            disk.write(curr, buffer);
            pos += bytes;

            if (pos < result.size()) {
                int16_t next = allocated_blocks[i+1];
                fat[curr] = next;
                curr = next;
            } else {
                fat[curr] = FAT_EOF;
            }
        }
    }
    save_fat();



    // Add to directory
    //discutera med Signe om det: parent_block = parent_entry.first_blk
    entries.push_back(entry);
    save_dir(parent_entry.first_blk, entries);
    //std::cout << "FS::create(" << filepath << ")\n";
    return 0;
}

//Thanh
int
FS::cat(std::string filepath) {
    dir_entry parent_entry{};
    string file_name;


    if (!resolve_path(filepath, parent_entry, file_name)) {
        cout << "File not found from path\n";
        return -1;
    }

    if (parent_entry.file_name[0] != '\0'){
        if (!check_rights(parent_entry, WRITE)) {
            cout << "Permission denied: cannot create file in directory " << parent_entry.file_name << endl;
            return -1;
        }
    }

    uint16_t parent_block = parent_entry.first_blk;
    vector<dir_entry> dir;

    if(load_dir(parent_block, dir) != 0){
        cout << "Could not load directory";
        return -1;
    }

    dir_entry file = {};
    bool found = false;
    for (auto& e : dir) {
        if (file_name == e.file_name) {
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

    if (!check_rights(file, READ)) {
        cout << "Permission denied with cat: cannot read file\n";
        return -1;
    }
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