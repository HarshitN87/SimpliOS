#include "ramdisk.h"
#include <stddef.h>

// External functions from kernel.c
extern void term_print(const char* str);
extern void term_putc(char c);

// String utility functions (freestanding environment)
static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Global ramdisk instance
ramdisk_t g_ramdisk;

// Helper function to print numbers
static void print_number(uint32_t num) {
    char buffer[12];
    int i = 0;
    
    if (num == 0) {
        term_putc('0');
        return;
    }
    
    // Convert number to string
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Print in reverse order
    while (i > 0) {
        term_putc(buffer[--i]);
    }
}

// Initialize the ramdisk
void ramdisk_init(void) {
    // Clear the entire ramdisk structure
    for (int i = 0; i < MAX_FILES; i++) {
        g_ramdisk.files[i].filename[0] = '\0';
        g_ramdisk.files[i].attributes.size = 0;
        g_ramdisk.files[i].attributes.creation_time = 0;
        g_ramdisk.files[i].attributes.modified_time = 0;
        g_ramdisk.files[i].attributes.type = FILE_TYPE_REGULAR;
        g_ramdisk.files[i].attributes.permissions = 0;
        g_ramdisk.files[i].data_offset = 0;
        g_ramdisk.files[i].in_use = 0;
    }
    
    // Clear data area
    for (int i = 0; i < RAMDISK_SIZE; i++) {
        g_ramdisk.data[i] = 0;
    }
    
    g_ramdisk.next_free_offset = 0;
    g_ramdisk.file_count = 0;
    
    term_print("Ramdisk initialized.\n");
}

// Find a file entry by name
file_entry_t* ramdisk_find_file(const char* filename) {
    if (!filename) return NULL;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_ramdisk.files[i].in_use && 
            strcmp(g_ramdisk.files[i].filename, filename) == 0) {
            return &g_ramdisk.files[i];
        }
    }
    return NULL;
}

// Find an empty file entry slot
file_entry_t* ramdisk_find_free_slot(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!g_ramdisk.files[i].in_use) {
            return &g_ramdisk.files[i];
        }
    }
    return NULL;
}

// Create a new file
int ramdisk_create_file(const char* filename, file_type_t type) {
    if (!filename || strlen(filename) >= MAX_FILENAME_LEN) {
        return -1; // Invalid filename
    }
    
    if (g_ramdisk.file_count >= MAX_FILES) {
        return -2; // No free file slots
    }
    
    // Check if file already exists
    if (ramdisk_find_file(filename)) {
        return -3; // File already exists
    }
    
    // Find free slot
    file_entry_t* entry = ramdisk_find_free_slot();
    if (!entry) {
        return -2; // No free slots
    }
    
    // Initialize file entry
    strcpy(entry->filename, filename);
    entry->attributes.size = 0;
    entry->attributes.creation_time = 0; // Will be set by caller
    entry->attributes.modified_time = 0;
    entry->attributes.type = type;
    entry->attributes.permissions = 0x66; // Read/write for all (truncated to fit uint8_t)
    entry->data_offset = g_ramdisk.next_free_offset;
    entry->in_use = 1;
    
    g_ramdisk.file_count++;
    
    return 0; // Success
}

// Delete a file
int ramdisk_delete_file(const char* filename) {
    file_entry_t* entry = ramdisk_find_file(filename);
    if (!entry) {
        return -1; // File not found
    }
    
    // Mark as unused
    entry->filename[0] = '\0';
    entry->in_use = 0;
    entry->data_offset = 0;
    entry->attributes.size = 0;
    
    g_ramdisk.file_count--;
    
    return 0; // Success
}

// Write data to a file
int ramdisk_write_file(const char* filename, const void* data, uint32_t size) {
    file_entry_t* entry = ramdisk_find_file(filename);
    if (!entry) {
        return -1; // File not found
    }
    
    if (size > MAX_FILE_SIZE) {
        return -2; // File too large
    }
    
    if (entry->data_offset + size > RAMDISK_SIZE) {
        return -3; // Not enough space in ramdisk
    }
    
    // Copy data to ramdisk
    const uint8_t* src = (const uint8_t*)data;
    uint8_t* dest = &g_ramdisk.data[entry->data_offset];
    
    for (uint32_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
    
    // Update file attributes
    entry->attributes.size = size;
    entry->attributes.modified_time = 0; // Will be set by caller
    
    return size; // Return number of bytes written
}

// Read data from a file
int ramdisk_read_file(const char* filename, void* buffer, uint32_t max_size) {
    file_entry_t* entry = ramdisk_find_file(filename);
    if (!entry) {
        return -1; // File not found
    }
    
    if (!buffer) {
        return -2; // Invalid buffer
    }
    
    uint32_t read_size = entry->attributes.size;
    if (read_size > max_size) {
        read_size = max_size;
    }
    
    // Copy data from ramdisk
    uint8_t* src = &g_ramdisk.data[entry->data_offset];
    uint8_t* dest = (uint8_t*)buffer;
    
    for (uint32_t i = 0; i < read_size; i++) {
        dest[i] = src[i];
    }
    
    return read_size; // Return number of bytes read
}

// List all files
int ramdisk_list_files(char* buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size < 1) {
        return -1;
    }
    
    buffer[0] = '\0';
    uint32_t offset = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_ramdisk.files[i].in_use) {
            const char* filename = g_ramdisk.files[i].filename;
            uint32_t name_len = strlen(filename);
            
            // Check if we have space
            if (offset + name_len + 2 >= buffer_size) {
                break; // No more space
            }
            
            // Copy filename
            strcpy(&buffer[offset], filename);
            offset += name_len;
            
            // Add newline
            buffer[offset++] = '\n';
            buffer[offset] = '\0';
        }
    }
    
    return offset;
}

// Get file size
uint32_t ramdisk_get_file_size(const char* filename) {
    file_entry_t* entry = ramdisk_find_file(filename);
    if (!entry) {
        return 0;
    }
    return entry->attributes.size;
}

// Create sample files for demonstration
void ramdisk_create_samples(void) {
    // Create welcome.txt
    if (ramdisk_create_file("welcome.txt", FILE_TYPE_REGULAR) == 0) {
        const char* welcome_text = "Welcome to SimpliOS!\nThis is a sample file in the ramdisk filesystem.\n";
        ramdisk_write_file("welcome.txt", welcome_text, strlen(welcome_text));
    }
    
    // Create readme.txt
    if (ramdisk_create_file("readme.txt", FILE_TYPE_REGULAR) == 0) {
        const char* readme_text = "SimpliOS Ramdisk Filesystem\n============================\n\nThis is an in-memory filesystem that stores files in RAM.\nUse the shell commands to manage files:\n- ls: list files\n- cat <file>: display file contents\n- create <file>: create new file\n- write <file> <text>: write text to file\n- read <file>: read file contents\n- delete <file>: delete file\n";
        ramdisk_write_file("readme.txt", readme_text, strlen(readme_text));
    }
    
    // Create version.txt
    if (ramdisk_create_file("version.txt", FILE_TYPE_REGULAR) == 0) {
        const char* version_text = "SimpliOS v0.1\nBuilt with love for learning OS development\n";
        ramdisk_write_file("version.txt", version_text, strlen(version_text));
    }
}

// Print ramdisk status
void ramdisk_print_status(void) {
    term_print("\n=== Ramdisk Status ===\n");
    term_print("Files stored: ");
    print_number(g_ramdisk.file_count);
    term_print("/");
    print_number(MAX_FILES);
    term_print("\n");
    
    term_print("Data used: ");
    print_number(g_ramdisk.next_free_offset);
    term_print("/");
    print_number(RAMDISK_SIZE);
    term_print(" bytes\n");
    
    if (g_ramdisk.file_count > 0) {
        term_print("Files:\n");
        for (int i = 0; i < MAX_FILES; i++) {
            if (g_ramdisk.files[i].in_use) {
                term_print("  ");
                term_print(g_ramdisk.files[i].filename);
                term_print(" (");
                print_number(g_ramdisk.files[i].attributes.size);
                term_print(" bytes)\n");
            }
        }
    }
    term_print("=====================\n");
}
