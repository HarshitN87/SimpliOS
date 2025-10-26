#ifndef RAMDISK_H
#define RAMDISK_H

#include <stdint.h>

// Filesystem constants
#define MAX_FILES 64
#define MAX_FILENAME_LEN 32
#define MAX_FILE_SIZE 4096
#define RAMDISK_SIZE (MAX_FILES * MAX_FILE_SIZE)

// File types
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY
} file_type_t;

// File attributes
typedef struct {
    uint32_t size;          // File size in bytes
    uint32_t creation_time; // Creation timestamp
    uint32_t modified_time; // Last modification timestamp
    file_type_t type;       // File type
    uint8_t permissions;    // File permissions (simplified)
} file_attributes_t;

// File entry in the ramdisk
typedef struct {
    char filename[MAX_FILENAME_LEN];  // File name
    file_attributes_t attributes;     // File attributes
    uint32_t data_offset;             // Offset in ramdisk data area
    uint8_t in_use;                   // Whether this entry is in use
} file_entry_t;

// Ramdisk structure
typedef struct {
    file_entry_t files[MAX_FILES];    // File entries
    uint8_t data[RAMDISK_SIZE];       // File data storage
    uint32_t next_free_offset;        // Next free offset in data area
    uint32_t file_count;              // Number of files currently stored
} ramdisk_t;

// Global ramdisk instance
extern ramdisk_t g_ramdisk;

// Function declarations
void ramdisk_init(void);
int ramdisk_create_file(const char* filename, file_type_t type);
int ramdisk_delete_file(const char* filename);
int ramdisk_write_file(const char* filename, const void* data, uint32_t size);
int ramdisk_read_file(const char* filename, void* buffer, uint32_t max_size);
int ramdisk_list_files(char* buffer, uint32_t buffer_size);
file_entry_t* ramdisk_find_file(const char* filename);
uint32_t ramdisk_get_file_size(const char* filename);
void ramdisk_print_status(void);
void ramdisk_create_samples(void);

#endif // RAMDISK_H
