#include <stdbool.h>

enum LabelType
{
    LABEL_ARM_CODE,
    LABEL_THUMB_CODE,
    LABEL_DATA,
    LABEL_POOL,
    LABEL_JUMP_TABLE,
};

enum BranchType
{
    BRANCH_TYPE_UNKNOWN,
    BRANCH_TYPE_B,
    BRANCH_TYPE_BL,
};

struct Label
{
    uint32_t addr;
    uint8_t type;
    uint8_t branchType;
    uint32_t size;
    bool processed;
    bool isFunc; // 100% sure it's a function, which cannot be changed to BRANCH_TYPE_B. 
    char *name;
};

extern struct Label *gLabels;
extern uint8_t *gInputFileBuffer;
extern size_t gInputFileBufferSize;
extern uint32_t ROM_LOAD_ADDR;

// disasm.c
int disasm_add_label(uint32_t addr, uint8_t type, char *name);
void disasm_disassemble(void);
