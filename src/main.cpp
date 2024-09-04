#include "gbadisasm.h"

#include <string.h>

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

struct ConfigLabel {
    uint32_t addr;
    uint8_t type;
    const char* label;
};

std::string gInputFile;
uint8_t* gInputFileBuffer;
size_t gInputFileBufferSize;
bool gStandaloneFlag;

static std::string read_to_string(std::string& fname){
    auto file = std::ifstream(fname, std::ios::in | std::ios::binary);
    if(!file.is_open()) fatal_error("could not open config file '%s'", fname.data());
    auto size = std::filesystem::file_size(fname);
    auto buffer = std::string(size, '\0');
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

static void read_input_file(std::string& fname) {
    gInputFile = read_to_string(fname);
    gInputFileBuffer = (uint8_t*)gInputFile.data();
    gInputFileBufferSize = gInputFile.size();
}

static char* split_word(char* s) {
    while(!isspace(*s)) {
        if(*s == '\0')
            return s;
        s++;
    }
    *s++ = '\0';
    while(isspace(*s))
        s++;
    return s;
}

static char* split_line(char* s) {
    while(*s != '\n' && *s != '\r') {
        if(*s == '\0')
            return s;
        s++;
    }
    *s++ = '\0';
    while(*s == '\n' || *s == '\r')
        s++;
    return s;
}

static char* skip_whitespace(char* s) {
    while(isspace(*s))
        s++;
    return s;
}

static void read_config(std::string& fname) {
    int lineNum = 1;

    auto buffer = read_to_string(fname);
    char* next;
    char* line;

    for(line = next = buffer.data(); *line != '\0'; line = next, lineNum++) {
        char* tokens[4];
        auto clone_cstring = [](char* src)->char* {
            if(src == nullptr || *src == '\0'){ return nullptr; }
            auto dest = (char*)malloc(strlen(src) + 1);
            strcpy(dest, src);
            return dest;
        };

        next = split_line(line);

        tokens[0] = line = skip_whitespace(line);
        for(auto i = 1; i < countof(tokens); i++)
            tokens[i] = line = split_word(line);

        if(tokens[0][0] == '#') // comment
            continue;
        if(strcmp(tokens[0], "arm_func") == 0) {
            int addr;
            int idx;

            if(sscanf(tokens[1], "%i", &addr) == 1) {
                idx = disasm_add_label(addr, LABEL_ARM_CODE, clone_cstring(tokens[2]));
                if(strlen(tokens[3]) != 0)
                    disasm_force_func(idx);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "thumb_func") == 0) {
            int addr;
            int idx;

            if(sscanf(tokens[1], "%i", &addr) == 1) {
                idx = disasm_add_label(addr, LABEL_THUMB_CODE, clone_cstring(tokens[2]));
                if(strlen(tokens[3]) != 0)
                    disasm_force_func(idx);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "thumb_label") == 0) {
            int addr;

            if(sscanf(tokens[1], "%i", &addr) == 1) {
                disasm_add_label(addr, LABEL_THUMB_CODE, clone_cstring(tokens[2]));
                disasm_set_branch_type(addr, BRANCH_TYPE_B, false);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "arm_label") == 0) {
            int addr;

            if(sscanf(tokens[1], "%i", &addr) == 1) {
                disasm_add_label(addr, LABEL_ARM_CODE, clone_cstring(tokens[2]));
                disasm_set_branch_type(addr, BRANCH_TYPE_B, false);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "thumb_far_jump") == 0) {
            int addr;

            if(sscanf(tokens[1], "%i", &addr) == 1) {
                disasm_add_label(addr, LABEL_THUMB_CODE, clone_cstring(tokens[2]));
                disasm_set_branch_type(addr, BRANCH_TYPE_B, true);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "arm_far_jump") == 0) {
            int addr;

            if(sscanf(tokens[1], "%i", &addr) == 1) {
                disasm_add_label(addr, LABEL_ARM_CODE, clone_cstring(tokens[2]));
                disasm_set_branch_type(addr, BRANCH_TYPE_B, true);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "pool_label") == 0) {
            int addr, count, i;

            if(sscanf(tokens[1], "%i", &addr) == 1
               && sscanf(tokens[2], "%i", &count) == 1) {
                for(i = 0; i < count; ++i)
                    disasm_add_label(addr + 4 * i, LABEL_POOL, NULL);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "jump_table") == 0) {
            int addr, count;

            if(sscanf(tokens[1], "%i", &addr) == 1
               && sscanf(tokens[2], "%i", &count) == 1) {
                disasm_add_label(addr, LABEL_JUMP_TABLE, NULL);
                if(jump_table_create_labels(addr, count))
                    fatal_error("%s: invalid params on line %i\n", fname, lineNum);
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else if(strcmp(tokens[0], "data_label") == 0) {
            int addr;
            if(sscanf(tokens[1], "%i", &addr) == 1) {
                disasm_add_label(addr, LABEL_DATA, clone_cstring(tokens[2]));
            } else {
                fatal_error("%s: syntax error on line %i\n", fname, lineNum);
            }
        } else {
            fprintf(stderr, "%s: warning: unrecognized command '%s' on line %i\n", fname, tokens[0], lineNum);
        }
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    std::string romFileName = "";
    std::string configFileName = "";
    ROM_LOAD_ADDR = 0x08000000;

    for(auto i = 1; i < argc; i++) {
        if(args[i] == "-c") {
            i++;
            if(i >= argc) fatal_error("expected filename for option -c");
            configFileName = args[i];
        } else if(args[i] == "-l") {
            i++;
            if(i >= argc) fatal_error("expected integer for option -l");
            ROM_LOAD_ADDR = std::stoul(args[i], nullptr, 0);
        } else if(args[i] == "-s") {
            gStandaloneFlag = true;
        } else {
            if(romFileName.empty()){
                romFileName = args[i];
            }else{
                fatal_error("Too many arguments / input file is defined mltiple times.");
            }
        }
    }

    if(romFileName.empty()) fatal_error("no ROM file specified");
    read_input_file(romFileName);
    if(!configFileName.empty()) read_config(configFileName);
    disasm_disassemble();
    return 0;
}
