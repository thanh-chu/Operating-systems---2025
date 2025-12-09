#include "fs.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>

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
    try {
        save_fat();
    } catch (const std::runtime_error& e){
        cerr << e.what() << endl;
    }
}


//check all FAT and return the first free block, help-funtion
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

    disk.write(ROOT_BLOCK, empty_block);
    for(int i = 2; i < BLOCK_SIZE/2; i++){
        disk.write(i, empty_block);
    }

    for(int i = 0; i < BLOCK_SIZE/2; i++){
        if(i == ROOT_BLOCK || i == FAT_BLOCK){
            fat[i] = FAT_EOF;
        } else {
            fat[i] = FAT_FREE;
        }
    }
    save_fat();
    
    cwd_block = ROOT_BLOCK;
    cwd_path = "/";
    return 0;
}


//function to check the right the access a directory/a file
bool FS::check_rights(const dir_entry &check_object, uint8_t rights) {
    return (check_object.access_rights & rights) == rights;
}

int FS::load_dir(uint16_t block_no, vector<dir_entry>& entries) {
    uint8_t buffer[BLOCK_SIZE];

    int res = disk.read(block_no, buffer);
    if (res != 0) {
        return res;
    }

    dir_entry* p = reinterpret_cast<dir_entry*>(buffer);
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
    if (dirpath.empty()) {
        cout << "Error: dirpath is not valid" << endl;
        return -1;
    }

    dir_entry parent_entry{};
    string new_name;
    if (!resolve_path(dirpath, parent_entry, new_name)) {
        cout << "Error: no such directory (parent) for: " << dirpath << endl;
        return -1;
    }

    if (new_name.size() > 55) {
        cout << "Error: filename is not valid," << endl;
        return -1;
    }

   //Check the access rights on a directory: WRITE with the parent directory
    if (parent_entry.first_blk != ROOT_BLOCK){
        if (!check_rights(parent_entry, WRITE)) {
            cout << "Permission denied: cannot create file in directory " << parent_entry.file_name << endl;
            return -1;
        }
    }

    uint16_t parent_block = parent_entry.first_blk;

    vector<dir_entry> parent_entries; // array att fylla
    int res = load_dir(parent_block, parent_entries);
    if (res < 0) {
        std::cout << "Error: could not load current directory\n";
        return -1;
    }

    for (size_t i = 0; i < parent_entries.size(); i++) {
        if (new_name == parent_entries[i].file_name) {
            cout << "Error: Directory already exists" << endl;
            return -1;
        }
    }

    if (parent_entries.size() >= DIR_ENTRIES) {
        cout << "Error: Directory full" << endl;
        return -1;
    }

    int new_block;
    try {
        new_block = alloc_block();
    } catch (const std::runtime_error& e) {
        cout << "Error: Disk is full\n";
        return -1;
    }

    dir_entry new_dir_entry{};
    strncpy(new_dir_entry.file_name, new_name.c_str(), sizeof(new_dir_entry.file_name) - 1);
    new_dir_entry.size = 0;
    new_dir_entry.first_blk = new_block;
    new_dir_entry.type = TYPE_DIR;
    new_dir_entry.access_rights = READ | WRITE | EXECUTE;

    parent_entries.insert(parent_entries.begin(), new_dir_entry);

    res = save_dir(parent_block, parent_entries);
    if (res < 0) {
        cout << "Error: could not save parent directory" << endl;
        return -1;
    }

    vector<dir_entry> new_entries;

    dir_entry back{};
    strncpy(back.file_name, "..", sizeof(back.file_name) - 1);
    back.size = parent_entry.size;
    back.first_blk = parent_entry.first_blk;
    back.type = TYPE_DIR;
    back.access_rights = parent_entry.access_rights;

    new_entries.insert(new_entries.begin(), back);

    res = save_dir(new_block, new_entries);
    if (res < 0) {
        cout << "Error: could not save new directory" << endl;
        return -1;
    }

    save_fat();
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls() {
    vector<dir_entry> entries;
    //ta ut hur många entries det finns

    int status_block = load_dir(cwd_block, entries); //leta upp
    if (status_block != 0) {
        cout << "Error: could not load directory block" << endl;
        return -1;
    }

    //check rights if not root
    if(cwd_block != ROOT_BLOCK){
        dir_entry parent;
        bool found_parent = false;
        for(auto& temp:entries){
            if (strcmp(temp.file_name, "..") == 0){
                parent = temp;
                found_parent = true;
                break;
            }
        }

        if(!found_parent){
            cout << "Error: not found parent directory.\n";
            return -1;
        }

        vector<dir_entry> parent_entries;
        if (load_dir(parent.first_blk, parent_entries) != 0) {
            cout << "Error: cannot load parent directory" << endl;
            return -1;
        }

        dir_entry cwd_entry;
        bool found = false;
        for(auto& temp: parent_entries){
            if(temp.first_blk == cwd_block){
                cwd_entry = temp;
                found = true;
                break;
            }
        }

        if (!found) {
            cout << "Error: cwd entry not found" << endl;
            return -1;
        }

        if (!check_rights(cwd_entry, READ)) {
            cout << "Error: permission denied to read current directory" << endl;
            return -1;
        }

    }
    
    cout << "name" << "\t" << "type"  << "\t" << "accessrights"  << "\t" <<"size" << endl;
    for (size_t i = 0; i < entries.size() ; i++) {
        string type = "";
        //run test4 with this so all file/directory has name ".." not show when run ls
        if(strcmp(entries[i].file_name, "..") == 0) continue;
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
int
FS::cd(string name) {
    dir_entry parent;
    string last_name;
    string path = name;

    if (!resolve_path(path, parent, last_name, true)) {
        cout << "Error: directory not found or permission denied to enter directory" << endl;
        return -1;
    }

    if (path == "/"){
        cwd_block = ROOT_BLOCK;
        cwd_path = path;
        return 0;
    }

    vector<dir_entry> entries;
    if (load_dir(parent.first_blk, entries) < 0) {
        cout << "Error: could not load directory\n";
        return -1;
    }

    for (auto &e : entries) {
        if (e.file_name == last_name) {
            if (e.type == TYPE_FILE) {
                cout << "Error: could not do cd on a file" << endl;
                return -1;
            }
            if (!check_rights(e, READ)) {
                cout << "Error: permission denied to read current directory" << endl;
                return -1;
            }
            cwd_block = e.first_blk;
            cwd_path  = path;
            return 0;
        }
    }

    cout << "Error: directory not found" << endl;
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
            for(size_t i = 0; i < entires.size(); i++){
                if(entires[i].file_name == name && entires[i].type == TYPE_FILE)
                {
                    if (name != parts.back()) {
                        if (throw_not_found) {
                            throw runtime_error("Error: Not a directory");
                        }
                        return false;
                    }
                    found_map = true;
                    break;
                }
              if(entires[i].type == TYPE_DIR && entires[i].file_name == name && check_rights(entires[i], EXECUTE) == true){
                if(name == parts.back()){
                    new_name = name;
                    found_map = true;
                    break;
                }
                entry = entires[i];
                current_block = entires[i].first_blk;
                found_map = true;
                continue;
             }
            }
            if (!found_map) {
                if (name != parts.back() || throw_not_found) {
                    return false;
                }
            } 
        }
    }
    return true;
}


// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    //retrieving info sourcepath
    dir_entry parent_entry_source = {};
    string filename_source;

    resolve_path(sourcepath, parent_entry_source, filename_source, true);

    //load directory source
    vector<dir_entry> entries_source;
    dir_entry to_move;
    load_dir(parent_entry_source.first_blk, entries_source);
    bool found = false;
    
    //find corresponding dir_entry
    int counter = 0;
    for(auto& entry: entries_source){
        if(entry.file_name==filename_source){
            to_move = entry;
            found = true;
            break;
        }
        counter++;
    }

    //Checking rights and type of source dir_entry
    if(found == false){
        cout << "Source not found" << endl;
        return -1;
    }

    if(!(to_move.access_rights & READ)){
        cout << "No access to copy sourcefile" << endl;
        return -1;
    }
    
    if((to_move.type != TYPE_FILE)){
        cout << "Can not copy directory, has to be file" << endl;
        return -1;
    }

    dir_entry parent_entry_dest = {};
    string filename_dest;

    //retrieving info destpath
    bool destExist = resolve_path(destpath, parent_entry_dest, filename_dest, true);
    vector<uint16_t> source_chain = get_chain(to_move.first_blk);

    //load directory for parent of dest directory to find dest directory
    vector<dir_entry> entries_dest;
    dir_entry dest_entry;
    load_dir(parent_entry_dest.first_blk, entries_dest);
    found = false;
    
    //Find corresponding dir_entry to destpath
    for(auto& entry: entries_dest){
        if(entry.file_name==filename_dest){
            dest_entry = entry;
            found = true;
            break;
        }
    }
    
    //Checking if dir_entry of destpath is directory, in that case need to load that directory
    vector<dir_entry> new_entries;
    if(found == true){
        if(!(dest_entry.type == TYPE_DIR)){
            cout << "File already exists" << endl;
            return -1;
        }
        //load directory for dest directory
        load_dir(dest_entry.first_blk, new_entries);
    
    } else {
        dest_entry = parent_entry_dest;
        new_entries = entries_dest;
    }

    //Checking rights and enough nr of free blocks
    if(!(dest_entry.access_rights & WRITE)&&!(dest_entry.first_blk==ROOT_BLOCK)){
        cout << "No access to move" << endl;
        return -1;
    }
    
    if(source_chain.size()> find_nr_of_free_blocks()){
        cout << "Not enough free space to copy file" << endl;
        return -1;
    }

    //Allocate new blocks for copied file, as many as in the sourcefile
    vector<uint16_t> new_blocks = {};
    for(int i = 0; i < source_chain.size(); i++){
        try{
            int new_alloc_block = alloc_block();
            new_blocks.push_back(new_alloc_block);
        } catch (const std::runtime_error& e) {
            cout << "disk is full\n";
            return -1;
        }
    }
    
    int16_t curr = new_blocks[0];
    size_t bytes = to_move.size;
    
    //Read from sourcefile and write to destfile
    for (size_t i = 0; i < new_blocks.size(); i++) {
        uint8_t buffer[BLOCK_SIZE] = {0};
        if (disk.read(source_chain[i], buffer) != 0) {
            cout << "disk read error\n"; return -1;
        }
        disk.write(curr, buffer);
        //adding new blocks to FAT
        if(i == new_blocks.size()-1){
            fat[curr] = FAT_EOF;
        } else {
            int16_t next = new_blocks[i+1];
            fat[curr] = next;
            curr = next;
        }
    }

    //Saving new dir_entry and setting filename if a new file is created.
    to_move.first_blk = new_blocks[0];

    //Checking new filename is not too long
    if(found == false){
        int n = filename_dest.length();
        if(n >= 55){
            cout << "Filename too long" << endl;
            return -1;
        }
        strcpy(to_move.file_name, filename_dest.c_str());
    }

    //Adding new dir_entry for copied file. Saving directory and FAT.
    new_entries.push_back(to_move);
    save_dir(dest_entry.first_blk, new_entries);
    save_fat();
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    dir_entry parent_entry_source = {};
    string filename_source;

    //retrieving info sourcepath
    resolve_path(sourcepath, parent_entry_source, filename_source, true);

    //load directory
    vector<dir_entry> entries_source;
    dir_entry to_move;
    load_dir(parent_entry_source.first_blk, entries_source);
    bool found = false;
    
    //Find dir_entry corresponding to sourcepath
    int counter = 0;
    for(auto& entry: entries_source){
        if(entry.file_name==filename_source){
            to_move = entry;
            found = true;
            break;
        }
        counter++;
    }

    //Checking rights and if source is found.
    if(found == false){
        cout << "Source not found" << endl;
        return -1;
    }

    if(!(to_move.access_rights & WRITE)){
        cout << "No access to move" << endl;
        return -1;
    }

    dir_entry parent_entry_dest = {};
    string filename_dest;

    //Retrieving information destpath
    resolve_path(destpath, parent_entry_dest, filename_dest, true);

    //load directory for parent of dest directory to find dest directory
    vector<dir_entry> entries_dest;
    dir_entry dest_entry;
    load_dir(parent_entry_dest.first_blk, entries_dest);
    found = false;
    
    //Find dir_entry corresponding to destpath
    for(auto& entry: entries_dest){
        if(entry.file_name==filename_dest){
            dest_entry = entry;
            found = true;
            break;
        }
    }
    vector<dir_entry> new_entries;
    //Checking if dir_entry of destpath is directory, in that case need to load that directory 
    if(found == true){
        if(!(dest_entry.type == TYPE_DIR)){
            cout << "File already exists" << endl;
            return -1;
        }
        load_dir(dest_entry.first_blk, new_entries);
    } else {
        dest_entry = parent_entry_dest;
        new_entries = entries_dest;
    }

    //Checking rights destpath dir_entry
    if(!(dest_entry.access_rights & WRITE)&&!(dest_entry.first_blk==ROOT_BLOCK)){
        cout << "No access to move" << endl;
        return -1;
    }

    //If dir_entry of destpath is not found, set new filename to filename in destpath
    if(found == false){
        int n = filename_dest.length();
        if(n >= 55){
            cout << "Filename too long" << endl;
            return -1;
        }
        strcpy(to_move.file_name, filename_dest.c_str());
    }

    //Checking if sourcepath and destpath in same folder
    if(dest_entry.first_blk==parent_entry_source.first_blk){
        //If in same folder, add to array of dir_entry
        for (int i = 0; i<new_entries.size(); i++)
        {
            if (new_entries[i].first_blk == to_move.first_blk)
            {
                new_entries[i] = to_move;
                break;
            }
        }
        save_dir(dest_entry.first_blk, new_entries);
    }
    else{
        //If not in same folder erase file in sourcepath and push it as entry in destfolder.
        entries_source.erase(entries_source.begin() + counter);
        new_entries.push_back(to_move);
        save_dir(parent_entry_source.first_blk, entries_source);
        save_dir(dest_entry.first_blk, new_entries);
    }
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    //Check if filepath exists.
    dir_entry parent_entry = {};
    string filename;

    //retrieving information filepath
    resolve_path(filepath, parent_entry, filename, true);

    //load directory
    vector<dir_entry> entries;
    dir_entry to_remove;
    load_dir(parent_entry.first_blk, entries);
    bool found = false;
    
    //Find corresponding dir_entry to filepath
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

    //Check accessrights
    if(!(to_remove.access_rights & WRITE)){
        cout << "No access to remove" << endl;
        return -1;
    }

    //If entry is dir, need to be empty to remove.
    vector<dir_entry> to_remove_entries;
    if(to_remove.type == TYPE_DIR){
        load_dir(to_remove.first_blk, to_remove_entries);
        bool only_parent = (to_remove_entries.size() == 1 &&
                        strcmp(to_remove_entries[0].file_name, "..") == 0);
        if (!only_parent) {
            cout << "Can not remove directory if not empty" << endl;
            return -1;
        }
    }

    //Remove from fat.
    free_chain(to_remove.first_blk);

    //Remove entry from vector of entries
    entries.erase(entries.begin() + counter);

    //save dir and fat
    save_dir(parent_entry.first_blk, entries);
    save_fat();
    
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    dir_entry parent_entry1 = {};
    string filename1;

    if(!resolve_path(filepath1, parent_entry1, filename1, true)){
        cout << "Can't find filepath1" << endl;
        return -1;
    };

    dir_entry parent_entry2 = {};
    string filename2;

    if(!resolve_path(filepath2, parent_entry2, filename2, true)){
        cout << "Can't find filepath2" << endl;
        return -1;
    };

    //load directory1
    vector<dir_entry> entries1;
    load_dir(parent_entry1.first_blk, entries1);

    //load directory2
    vector<dir_entry> entries2;
    load_dir(parent_entry2.first_blk, entries2);

    //Find corresponding dir_entry
    bool found;
    dir_entry* file_to_add_to;
    for(auto& entry: entries2){
        if(entry.file_name==filename2){
            file_to_add_to = &entry;
            found = true;
            break;
        }
    }
    if(found == false){
        cout << "Entry not found" << endl;
        return -1;
    }
    
    dir_entry file_to_add;
    for(auto& entry: entries1){
        if(entry.file_name==filename1){
            file_to_add = entry;
            found = true;
            break;
        }
    }
    if(found == false){
        cout << "Entry not found" << endl;
        return -1;
    }
    
    //Check access & type
    if(!(file_to_add.type == TYPE_FILE && file_to_add_to->type == TYPE_FILE)){
        cout << "Cant append directory" << endl;
        return -1;
    }
    if(!(file_to_add_to->access_rights & WRITE)){
        cout << "No access to write to file2" << endl;
        return -1;
    }
    if(!(file_to_add.access_rights & READ)){
        cout << "No access to read file1" << endl;
        return -1;
    }

    string to_add; //string to add to file 2, consists of content in last block + content of blocks in file 1
    int nr_of_whole_blocks_filled_by_file2 = floor(file_to_add_to->size/BLOCK_SIZE);
    int chars_in_last_block_file_2 = file_to_add_to->size % BLOCK_SIZE;
    int chars_left_in_last_block_file_2 = BLOCK_SIZE - chars_in_last_block_file_2;
    
    //Adding content of last block of file 2 to string to_add
    vector<int> allocated_blocks = {};
    int16_t blk = file_to_add_to->first_blk;
    int16_t last_blk = blk;
    while(fat[blk] != EOF){
        allocated_blocks.push_back(blk);
        last_blk = fat[blk];
        blk = fat[blk];
    }
    allocated_blocks.push_back(last_blk);
    uint8_t buffer[BLOCK_SIZE] = {0};
    if (disk.read(blk, buffer) != 0) {
        cout << "disk read error\n"; return -1;
    }
    to_add += string((char*)buffer, chars_in_last_block_file_2);
    
    int remaining_chars = file_to_add.size;
    blk = file_to_add.first_blk;

    //Adding the contents of file1 to string to_add
    while (remaining_chars > 0) {
        uint8_t buffer[BLOCK_SIZE] = {0};
        if (disk.read(blk, buffer) != 0) {
            cout << "disk read error\n"; return -1;
        }
        int to_print = min(remaining_chars, BLOCK_SIZE);
        to_add += string((char*)buffer, to_print);
        remaining_chars -= to_print;
        if (remaining_chars > 0) blk = fat[blk];
    }

    //Check enough nr of free blocks
    if(nr_of_whole_blocks_filled_by_file2 > find_nr_of_free_blocks()){
        cout << "Disk is full" << endl;
        return -1;
    }

    vector<uint16_t> chain_blocks_file2 = get_chain(file_to_add_to->first_blk);
    uint16_t last_blk_to_add_to = chain_blocks_file2.back();

    int size = to_add.length(); //check how many bytes needed for the file
    div_t needed_blocks = div(size,BLOCK_SIZE); //check how many blocks are needed
    int whole_blocks_needed = needed_blocks.quot;
    if(needed_blocks.rem > 0){
        whole_blocks_needed++;
    }

    int16_t curr = last_blk_to_add_to; //current last block which we want to overwrite.
    vector<int16_t> added_blocks = {}; //new blocks including last block of file2
    added_blocks.push_back(last_blk_to_add_to);

    //Allocating new blocks
    for (size_t i = 0; i < whole_blocks_needed-1; i++){ //needed blocks -1 bc first block already allocated
        try {
            int new_alloc_block = alloc_block();
            added_blocks.push_back(new_alloc_block);
        } catch (const std::runtime_error& e) {
            cout << "disk is full\n";
            return -1;
        }

    }
    size_t pos = 0;

    //Writing to disk
    for (int i = 0; i < added_blocks.size(); i++) {
        uint8_t buffer[BLOCK_SIZE] = {0};
        size_t bytes = min((size_t)BLOCK_SIZE, to_add.size() - pos);
        memcpy(buffer, &to_add[pos], bytes);
        disk.write(curr, buffer);
        pos += bytes;

        if (pos < to_add.size()) {
            int16_t next = added_blocks[i+1];
            fat[curr] = next;
            curr = next;
        } else {
            fat[curr] = FAT_EOF;
        }
    }

    //Saving
    file_to_add_to->size = file_to_add_to->size + file_to_add.size;
    save_dir(parent_entry2.first_blk, entries2);
    save_fat();
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    int accessrights_as_int;
    //Check accessrights is correct format
    try{
        accessrights_as_int = stoi(accessrights);
    } catch(const std::invalid_argument &e) {
        std::cout << "Error: Wrong format accessrights" << std::endl;
        return -1;
    }

    if(accessrights_as_int>7 || accessrights_as_int < 0){
        std::cout << "Error: Wrong format accessrights" << std::endl;
        return -1;
    }

    dir_entry parent_entry{};
    string file_name;

    //Checking path
    if (!resolve_path(filepath, parent_entry, file_name)) {
        cout << "Error: File not found from path\n";
        return -1;
    }

    vector<dir_entry> dir;
    if (load_dir(parent_entry.first_blk, dir) != 0) {
        cout << "Error: could not load current directory\n";
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
    if (!found) { cout << "Error: File not found\n"; return -1; }

    save_dir(parent_entry.first_blk, dir);
    return 0;
}


// find_nr_of_free_blocks is used as a check before adding new files
//  to know the number of free blocks at that time.
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

    //Retrieving information filepath
    if (!resolve_path(filepath, parent_entry, filename)) {
        cout << "Invalid path: cannot resolve parent directory\n";
        return -1;
    }

    //Check rights
    if (parent_entry.first_blk != ROOT_BLOCK){
        if (!check_rights(parent_entry, WRITE)) {
            cout << "Insufficient access rights" << parent_entry.file_name << endl;
            return -1;
        }
    }

    //Check length filename
    if(filename.size() > 55){
        std::cout << "Error: Filename too long" << std::endl;
        return -1;
    }

    vector<dir_entry> entries;

    //Loading directory of filepath. Checking lnr of direntries and file not already existing
    if(load_dir(parent_entry.first_blk, entries) != 0){
        cout << "Error: Could not load directory";
        return -1;
    }

    if (entries.size() >= DIR_ENTRIES) {
        cout << "Error: Directory full\n";
        return -1;
    }

    for (size_t i = 0; i < entries.size(); i++) {
        if (filename == entries[i].file_name) {
            cout << "Error: File already exists" << endl;
            return -1;
        }
    }

    //Retrieving content text from terminal, saving it to string result
    std::string buf, result = {};
    while (getline(std::cin, buf)) //read std::cin into buf
    {
        if (buf.empty()) // if buf is empty break loop
            break;
        result += buf + "\n";
    }
    int needed_blocks;
    size_t size = result.length(); //check how many bytes needed for the file
    if(size == 0){
        needed_blocks = 1; //Always allocate 1 block
    } else {
        div_t needed_blocks_div = div(size,BLOCK_SIZE); //check how many blocks are needed
        needed_blocks = needed_blocks_div.quot;
        if(needed_blocks_div.rem > 0){
            needed_blocks++;
        }
    }   


    //create new dir_entry
    dir_entry entry = {};
    strncpy(entry.file_name, filename.c_str(), sizeof(entry.file_name));
    entry.type = TYPE_FILE;
    entry.access_rights = READ|WRITE;
    entry.size = size;
    entry.first_blk = FAT_EOF;


    vector<int> allocated_blocks = {};
    //Check enough free blocks for writing file content to disk.
    if(needed_blocks > static_cast<size_t>(find_nr_of_free_blocks())){
        std::cout << "Error: Disk is full" << std::endl;
        return -1;
    }
    //Allocate blocks
    for (size_t i = 0; i < needed_blocks; i++){
        try {
            int new_alloc_block = alloc_block();
            allocated_blocks.push_back(new_alloc_block);
        } catch (const std::runtime_error& e) {
            cout << "Error: disk is full\n";
            return -1;
        }

    }
    entry.first_blk = allocated_blocks[0];

    int16_t curr = allocated_blocks[0];
    size_t pos = 0;

    //Write the content of string result to disk, one block at a time
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

    //save updated directory and fat
    save_fat();
    entries.push_back(entry);
    save_dir(parent_entry.first_blk, entries);
    return 0;
}


int
FS::cat(std::string filepath) {
    dir_entry parent_entry{};
    string file_name;

    //retrieving information filepath
    if (!resolve_path(filepath, parent_entry, file_name)) {
        cout << "Error: File not found from path\n";
        return -1;
    }

    uint16_t parent_block = parent_entry.first_blk;
    vector<dir_entry> dir;

    //Loading directory of filepath
    if(load_dir(parent_block, dir) != 0){
        cout << "Error: Could not load directory";
        return -1;
    }

    dir_entry file = {};
    bool found = false;
    //Finding corresponding dir_entry to filepath, checking if already existing and not a directory
    for (auto& e : dir) {
        if (file_name == e.file_name) {
            if(e.type == TYPE_DIR){
                cerr << "Error: Try to use cat with directory";
                return -1;
            } else {
                file = e; found = true;
                break;
            }
        }
    }

    //Checking rights and if not found, printing error messages
    if (!found) { cout << "Error: File not found\n"; return -1; }

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

    //Going through file blocks, reading from disk one block at a time, printing to terminal
    while (remaining > 0) {
        if (blk == FAT_EOF) {
            cout << "Corrupted file: unexpected end of FAT chain\n";
            return -1;
        }

        uint8_t buffer[BLOCK_SIZE] = {0};
        if (disk.read(blk, buffer) != 0) {
            cout << "Error: disk read error\n"; 
            return -1;
        }

        int to_print = min(remaining, BLOCK_SIZE);
        cout << string((char*)buffer, to_print);
        remaining -= to_print;

        blk = fat[blk]; 
    }

    return 0;
}