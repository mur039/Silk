#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main(int argc, char** argv){
    
    if(argc < 2){
        puts("Expected file path\n");
        return 1;
    }

    const char* modulepath = argv[1];
    FILE* f = fopen(modulepath, "rb");
    if(!f){
        puts("Failed to open file\n");
        return 2;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    rewind(f);

    
    void* area = mmap(NULL, file_size, 0, MAP_ANONYMOUS, -1, 0 );
    fread(area, 1, file_size, f);
    
    long err = syscall(SYSCALL_INIT_MODULE, area, file_size, NULL);
    if(err < 0){
        perror("failed to load module");
        return 3;
    }

    fclose(f);
    free(area);
    return 0;
}