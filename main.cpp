//
// Command-line utility:
// Converts files to LF Format using Win32.
//

#include <windows.h>
#include <shellapi.h>

#define STDOUT_BUFFER_SIZE 16384
#define Panic(msg) (__panic(msg))

HANDLE stdout = INVALID_HANDLE_VALUE;
char stdout_buffer[STDOUT_BUFFER_SIZE] = {};
int stdout_buffer_size = 0;

BOOL IsDirectory(LPWSTR DirectoryOrFile) {
    DWORD FileAttribs = GetFileAttributesW(DirectoryOrFile);
    if (FileAttribs & FILE_ATTRIBUTE_DIRECTORY)
        return TRUE;
    else
        return FALSE;
}

void Concatenate(char *dst, const char *src) {
    while (*dst) ++dst;
    // We are at the null terminator now.
    
    // Copy from src to dst.
    while (*src) *dst++ = *src++;
    
    // Add the null-terminator.
    *dst = 0;
}

size_t StringLength(const char *str) {
    const char *s = str;
    while (*s) ++s;
    
    return s-str;
}

void __panic(const char *msg) {
    char full_msg[2048] = {};
    Concatenate(full_msg, "ERROR: ");
    Concatenate(full_msg, msg);
    Concatenate(full_msg, "  Exiting...");
    
    WriteFile(stdout,
              full_msg,
              StringLength(full_msg),
              NULL,
              NULL);
    ExitProcess(1);
}

void PushStdoutBuffer() {
    BOOL ok = WriteFile(stdout,
                        stdout_buffer,
                        stdout_buffer_size,
                        NULL,
                        NULL);
    if (!ok) MessageBoxA(NULL, "Couldn't write to standard output!", "Error!", MB_OK|MB_ICONERROR);
}

// Buffering since standard output is slow AF.
void Print(const char *message) {
    int length = StringLength(message);
    if (length > STDOUT_BUFFER_SIZE) Panic("Print message too long!");
    
    if (StringLength(message) + stdout_buffer_size >= STDOUT_BUFFER_SIZE) {
        PushStdoutBuffer();
        
        for (int i = 0; i < STDOUT_BUFFER_SIZE; i++) stdout_buffer[i] = 0;
        stdout_buffer_size = 0;
    }
    
    Concatenate(stdout_buffer, message);
    stdout_buffer_size += length;
}

typedef char CHARACTER;

void normalize_file(char *filename) {
    HANDLE file = CreateFileA(filename,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (file == INVALID_HANDLE_VALUE) {
        char message[MAX_PATH] = {};
        Concatenate(message, "Couldn't open ");
        Concatenate(message, filename);
        Concatenate(message, "\n");
        Print(message);
        return;
    }
    
    DWORD FileSize = GetFileSize(file, NULL);
    DWORD BytesRead = 0;
    
    CHARACTER *original = (CHARACTER*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHARACTER) * FileSize);
    if (!original) Panic("Allocation error!");
    
    BOOL ok = ReadFile(file,
                       (void*)original,
                       FileSize,
                       &BytesRead,
                       NULL);
    if (!ok) Panic("File couldn't be read!");
    
    if (BytesRead != FileSize) Panic("Bytes Read != File Size!");
    
    CHARACTER *output = (CHARACTER*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHARACTER) * FileSize);
    if (!output) Panic("Allocation error!");
    DWORD size = 0;
    
    for (DWORD i = 0; i < BytesRead; i++) {
        CHARACTER c = original[i];
        
        // Handling CR
        if (c == (CHARACTER)'\r' && ((i+1<BytesRead && original[i+1] != (CHARACTER)'\n') || (i+1>=BytesRead))) {
            output[size++] = (CHARACTER)'\n';
            i++;
        }
        
        // Handling CRLF
        else if (c == (CHARACTER)'\r' && original[i+1] == (CHARACTER)'\n') {
            output[size++] = (CHARACTER)'\n';
            i++;
        }
        
        // c is a normal character or a plain LF
        else {
            output[size++] = c;
        }
    }
    
    CloseHandle(file);
    file = 0;
    
    {
        file = CreateFileA(filename,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (file == INVALID_HANDLE_VALUE) {
            char message[MAX_PATH] = {};
            Concatenate(message, "Couldn't open ");
            Concatenate(message, filename);
            Concatenate(message, "\n");
            Print(message);
            return;
        }
        
        DWORD BytesWritten = 0;
        
        ok = WriteFile(file,
                       (void*)output,
                       size,
                       &BytesWritten,
                       NULL);
        if (!ok) Panic("Error with writing to file!");
        
        SetEndOfFile(file);
        
        CloseHandle(file);
    }
    
    HeapFree(GetProcessHeap(), 0, original);
    HeapFree(GetProcessHeap(), 0, output);
    
    char message[MAX_PATH] = {};
    Concatenate(message, filename);
    Concatenate(message, "\n");
    
    Print(message);
}

int mainCRTStartup(void) {
    LPWSTR CommandLine = GetCommandLineW();
    int argc = 0;
    
    stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout == INVALID_HANDLE_VALUE || stdout == NULL)
        MessageBoxA(NULL, "Couldn't get standard output!", "Error!", MB_OK|MB_ICONERROR);
    
    LPWSTR *args = CommandLineToArgvW(CommandLine, &argc);
    
    Panic("Something went wrong :(");
    
    if (argc == 1) {
        Print("Usage: convertlf <pattern>\n  Example: convertlf *.c *.h\n  Example: convertlf c:\\test\\*");
        PushStdoutBuffer();
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        WIN32_FIND_DATA Data;
        char pattern[1024] = {};
        
        wchar_t *s = args[i];
        int j = 0;
        while (*s) {
            pattern[j] = (char)*s;
            j++;
            ++s;
        }
        
        int pattern_size = StringLength(pattern);
        
        HANDLE Find = FindFirstFile(pattern, &Data);
        if (Find == INVALID_HANDLE_VALUE) Panic("Invalid handle value :(");
        
        do {
            if (!(Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                //MessageBox(NULL, Data.cFileName, "File hit", MB_OK);
                normalize_file(Data.cFileName);
            }
        } while (FindNextFileA(Find, &Data));
        
        FindClose(Find);
    }
    
    PushStdoutBuffer();
    return 0;
}