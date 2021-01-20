#include "3bc.h"

#define print_file(file,type, val);\
switch(type){\
case STRI: fprintf(file, "%lu", (unsigned long) val); break;\
case STRO: fprintf(file, "%o", (unsigned int) val); break;\
case STRC: fprintf(file, "%c", (unsigned char) val); break;\
case STRX: fprintf(file, "%x", (unsigned int) val); break;\
case STRU: fprintf(file, "%d", (signed int) val); break;} 

#define print_error(string) fprintf(stderr, "> ERROR DESCRIPTION: %s\n", string);break

#ifndef _WIN32
struct termios term_old_attr;
struct termios term_new_attr;
#endif

file_t* program_file;

void lang_driver_run()
{
    while(tape_program_avaliable()? tape_program_exe(): lang_interpreter_line(program_file));
}

void lang_driver_init(int argc, char **argv)
{
    signal(SIGINT, lang_driver_exit);

    if (argc <= 1) {
        program_file = stdin;
    }
    else {
        program_file = fopen(argv[argc - 1], "r");
    }

    #ifndef _WIN32
    tcgetattr(0, &term_old_attr);
    tcgetattr(0, &term_new_attr);

    term_new_attr.c_lflag &= ~ICANON;
    term_new_attr.c_lflag &= ~ECHO;

    term_new_attr.c_cc[VTIME] = 0;
    term_new_attr.c_cc[VMIN] = 1;
    #endif
}

void lang_driver_exit(int sig)
{
    #ifndef _WIN32
    tcsetattr(STDIN_FILENO,TCSANOW,&term_old_attr);
    #endif

    if (program_file != stdin) {
        fclose(program_file);
    }

    tape_memory_destroy();
    tape_program_destroy();
    tape_sort_destroy();

    exit(sig);
}

void lang_driver_output_1(reg_t type, val_t val)
{
    print_file(stdout, type, val);
}

void lang_driver_output_2(reg_t type, val_t val)
{
    print_file(stderr, type, val);
}

void lang_driver_error(error_t error_code)
{
    fprintf(stderr, "\n[3BC] CRITICAL ERROR ABORTED THE PROGRAM");
    fprintf(stderr, "\n> ERROR LINE: %d", CLINE + 1);
    fprintf(stderr, "\n> ERROR CODE: %d\n", error_code);

    switch(error_code)
    {
        case ERROR_CPU_ZERO: print_error("EMPUTY CPU MODE"); 
        case ERROR_CPU_UNDEF: print_error("UNDEFINED CPU MODE");
        case ERROR_CPU_PROTECT: print_error("PROTECTED CPU MODE");
        case ERROR_CPU_RESERVED: print_error("RESERVED CPU MODE");
        case ERROR_CPU_REGISTER: print_error("UNDEFINED CPU REGISTER");
        case ERROR_CPU_INVALID: print_error("INVALID CPU MODE");
        case ERROR_INVALID_LABEL: print_error("INVALID LABEL");
        case ERROR_INVALID_VALUE: print_error("INVALID VALUE");
        case ERROR_INVALID_ADDRESS: print_error("INVALID ADDRESS");
        case ERROR_PARAM_DUALITY: print_error("DUALITY ADDRES WITH VALUE IS NOT ALLOWED");
        case ERROR_PARAM_REQUIRE_VALUE: print_error("VALUE IS REQUIRED");
        case ERROR_PARAM_REQUIRE_ADDRESS: print_error("ADDRESS IS REQUIRED");
        case ERROR_PARAM_BLOCKED_VALUE: print_error("VALUE IS NOT ALLOWED");
        case ERROR_PARAM_BLOCKED_ADDRESS: print_error("ADDRESS IS NOT ALLOWED");
        case ERROR_INTERPRETER_REGISTER: print_error("INVALID REGISTER");
        case ERROR_INTERPRETER_NUMBER: print_error("INVALID NUMBER");
        case ERROR_TAPE_LABEL: print_error("FAILURE TO EXPAND THE LABEL LIST");
        case ERROR_TAPE_MEMORY: print_error("FAILURE TO EXPAND THE MEMORY");
        case ERROR_TAPE_PROGRAM: print_error("FAILURE TO EXPAND THE PROGRAM");
        case ERROR_TAPE_SORT: print_error("FAILURE TO EXPAND THE SORT");
        case ERROR_INVALID_MEMORY_CONFIG: print_error("INVALID MEMORY TYPE CONFIG");
        case ERROR_INVALID_MEMORY_CLAMP:  print_error("INVALID MEMORY TYPE CLAMP");
        case ERROR_VOID_HELPER_MAX_MIN: print_error("MAX/MIN CANNOT BE EMPTY");
        default: print_error("UNKNOWN ERROR");
    }

    lang_driver_exit(EXIT_FAILURE);
}

/**
 * detect keyboard input
 */
val_t lang_driver_input(reg_t type, mem_t addres)
{
    static unsigned int value;
    static char c[2] = "\0";
    static bool invalid;
    
    do {
        invalid = false;

        /** capture input **/
        #ifndef _WIN32
        tcsetattr(STDIN_FILENO,TCSANOW, &term_new_attr);
        c[0] = getchar();
        tcsetattr(STDIN_FILENO,TCSANOW, &term_old_attr);
        #else 
        c[0] = getch();
        #endif

        /** validate input **/
        switch (type) {
            case STRC: 
                /** verify is a ASCII alphabet's letter/symbol **/
                invalid |= (c[0] < 0x20 || c[0] > 0x7E);
                value = c[0];
                break;

            case STRI: 
                invalid |= !sscanf(c, "%d", &value);
                break;
            
            case STRO: 
                invalid |= !sscanf(c, "%o", &value);
                break;

            case STRX:
                invalid |= !sscanf(c, "%x", &value);
                break;
        }

        /** validade input inner memory clamp limits **/
        if ((tape_memory_type_get(addres) & MEM_CONFIG_MIN_VALUE) == MEM_CONFIG_MIN_VALUE) {
            invalid |= tape_memory_value_min_get(addres) > value;
        }
        if ((tape_memory_type_get(addres) & MEM_CONFIG_MAX_VALUE) == MEM_CONFIG_MAX_VALUE) {
            invalid |= tape_memory_value_max_get(addres) < value;
        }
    
    }
    while (invalid);

    return (val_t) value;
}