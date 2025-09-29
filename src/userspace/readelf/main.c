
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>


enum options{
    OPTION_ONLY_HEADER = 0,
    OPTION_EOT
};




const char* single_dash_options[] = {
    
    [OPTION_ONLY_HEADER] = "-h",
    [OPTION_EOT] = NULL
};

const char* double_dash_options[] = {
    
    [OPTION_ONLY_HEADER] = "--file-header",
    [OPTION_EOT] = NULL
};

const int need_options_arg[] = {
    
    [OPTION_ONLY_HEADER] = 1,
    [OPTION_EOT] = -1
};



//both str and list must be null terminated
int is_string_in_string_list(const char* str, const char** list){

    if( !(str && list) ) //one of them is null
        return -1;
    
    for(int i = 0; list[i] ; ++i){

        //if we find it here we return it

        if(!memcmp(str, list[i], strlen(list[i]) )) //we find it
            return i;
    }       

    //otherwise
    return -1;
}


const char help_string[] = 
                            "Usage:\n"
                            "\treadelf [option(s)] <elf_file>:\n"
                            "\nOptions:\n"
                            "-h, --file-header         display the ELF file header\n"
                            ;

void print_help_str(){
    puts(help_string);
}



#define ELF_MAGIC 0x7f454c46
const uint8_t elf_magic_str[4] = { 0x7f, 0x45, 0x4c, 0x46};

#define EI_NIDENT 16
typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;


const char *elf_class_type[3] = {
                                    "INVALID CLASS",
                                    "ELF32",
                                    "ELF64"
                                };


const char *elf_data_type[3] = {
                                    "INVALID DATA ENCODING",
                                    "2' s complement, little endian",
                                    "2' s complement, big endian"
                                };

const char *elf_object_file_type[5] = {
                                    "NONE (huh?)",
                                    "REL (Relocatable file)",
                                    "EXEC (Executable file)",
                                    "DYN (Shared object file)",
                                    "CORE (Core file)"
                                };

const char *elf_machine_type[6] = {
                                    "NONE (huh?)",
                                    "AT&T WE 32100",
                                    "Sun SPARC",
                                    "Intel 80386",
                                    "Motorola m68k family",
                                    "Motorola m88k family"
                                };




int main(int argc, char **argv){
    

    int src_target_index = 0;
    char* src_target_table[1];
    const char *source = NULL;
    int operation = -1;
    if(argc == 1){
        puts("Expected arguments, see --help");
        return 1;
    }


    for(int i = 1; i < argc; ++i){
        
        int index = -1;
        if( !memcmp( argv[i], "--", 2 ) ){
            
            index = is_string_in_string_list(argv[i], double_dash_options);
        }else if( !memcmp( argv[i], "-", 1 ) ){
            
            index = is_string_in_string_list(argv[i], single_dash_options);
        }
        else{ //probably source or target
            index = -2;

        }



        switch(index){

            case -2:
                if(src_target_index < 1)
                    src_target_table[src_target_index++] = argv[i];
                else{
                    puts("mount: Bad usage try 'readelf --help'\n");
                    return 1;
                }
                break;

            case -1:
                printf("Unknown option \"%s\"\n", argv[i]);
                break;

            case OPTION_ONLY_HEADER:
                operation = OPTION_ONLY_HEADER;
                break;

            default:break;

        }
    }

    source = src_target_table[0];
    if(operation == -1){
        print_help_str();
        return 1;
    }

    if(!source){
        puts("ELF file is not specified...\n");
        return 1;
    }

    //check for the eld file
    int file_fd = open(source, O_RDONLY);
    if(file_fd == -1){
        puts("Failed to open file\n");
        return 1;
    }

    Elf32_Ehdr header;
    read(file_fd, header.e_ident, 16); //read e_ident first
    

    uint32_t *dword_ptr = (uint32_t*)&header;
    if( memcmp(header.e_ident, elf_magic_str, 4) != 0){
        puts("readelf: Error: Not an ELF executable - it doesn't contain valid magic bytes\n");
        return 1;
    }

    //then load rest of the header
    lseek(file_fd, 0, 0);
    read(file_fd, &header, sizeof(Elf32_Ehdr)); //read whole header


    puts("ELF Header:\n");
    puts("\tMagic:\t");
    for(int i = 0; i < 16; ++i){
        printf("%x ", header.e_ident[i]);
    }
    putchar('\n');

    printf("\tClass:                         %s\n", elf_class_type[header.e_ident[4] ] );
    printf("\tData:                          %s\n", elf_data_type[ header.e_ident[5] ] );
    printf("\tVersion:                       %u\n",header.e_ident[6]);

    printf("\tOS/ABI:                       ");
    switch(header.e_ident[7]){
        case 0:   puts("UNIX -System V\n"); break;
        case 1:   puts("HP-UX\n");break;
        case 2:   puts("NetBSD\n");break;
        case 3:   puts("Linux\n");break;
        case 4:   puts("GNU Hurd\n");break;
        
        case 6:   puts("Solaris\n");break;
        case 7:   puts("IBM AIX\n");break;
        case 8:   puts("SGI Irix\n");break;
        case 9:   puts("FreeBSD\n");break;
        case 10:  puts("Compaq TRU64\n");break;
        case 11:  puts("Novell Modesto\n");break;
        case 12:  puts("OpenBSD\n");break;
        default:  puts("UNKOWN\n");break;
    }
    printf("\tABI Version:                   %u\n",header.e_ident[8]);

    printf("\tType:                          %s\n",elf_object_file_type[header.e_type % 5]);
    printf("\tMachine:                       %s\n",elf_machine_type[header.e_machine]);
    printf("\tVersion:                       %x\n",header.e_version);
    printf("\tEntry point address:           %x\n",header.e_entry);
    printf("\tStart of program headers:      %u (bytes into file)\n",header.e_phoff);
    printf("\tStart of section headers:      %u (bytes into file)\n",header.e_shoff);
    printf("\tFlags:\t\t\t%x\n",header.e_flags);








    

            
    





    
    
    close(file_fd);
    return 0;
}
