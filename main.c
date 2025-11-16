    #include "include/disk.h"
    #include "include/fs.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #define VIRTUAL_DISK_PATH "mfs_test_disk.img"
    #define TOTAL_BLOCKS 100 // A small disk size for testing (400KB)
    #define TEST_CONTENT "The quick brown fox jumps over the lazy dog in my new file system."
    #define CONTENT_LENGTH (sizeof(TEST_CONTENT) - 1) // Exclude null terminator for raw data

    // --- Helper Functions ---

    void check_result(const char *action, ssize_t result, ssize_t expected_min) {
        if (result < expected_min) {
            fprintf(stderr, "[FAILURE] %s: Expected result >= %zd, got %zd.\n", action, expected_min, result);
            exit(EXIT_FAILURE);
        }
        printf("[SUCCESS] %s (Result: %zd)\n", action, result);
    }

    void check_pointer(const char *action, const void *ptr) {
        if (ptr == NULL) {
            fprintf(stderr, "[FAILURE] %s: Pointer is NULL. Aborting.\n", action);
            exit(EXIT_FAILURE);
        }
        printf("[SUCCESS] %s\n", action);
    }

    // --- Main Program ---

    int main() {
        Disk *disk = NULL;
        FileSystem fs = {0}; // Initialize all members to zero/NULL
        ssize_t inode_num = -1;
        char read_buffer[BLOCK_SIZE] = {0};

        printf("--- My File System Test ---\n");
        printf("Disk Size: %u blocks (%zu bytes)\n\n", TOTAL_BLOCKS, TOTAL_BLOCKS * BLOCK_SIZE);

        // 1. DISK OPEN & INITIALIZATION
        printf("--- 1. Disk Setup ---\n");
        disk = disk_open(VIRTUAL_DISK_PATH, TOTAL_BLOCKS);
        check_pointer("disk_open", disk);

        // 2. FORMAT THE DISK
        printf("\n--- 2. Formatting ---\n");
        if (!fs_format(disk)) {
            fprintf(stderr, "[FAILURE] fs_format failed. Aborting.\n");
            goto cleanup;
        }
        printf("[SUCCESS] fs_format\n");

        // 3. MOUNT THE FILE SYSTEM
        printf("\n--- 3. Mounting ---\n");
        if (!fs_mount(&fs, disk)) {
            fprintf(stderr, "[FAILURE] fs_mount failed. Aborting.\n");
            goto cleanup;
        }
        printf("[SUCCESS] fs_mount\n");

        // 4. CREATE A NEW INODE (File)
        printf("\n--- 4. File Creation ---\n");
        inode_num = fs_create(&fs);
        check_result("fs_create (get inode number)", inode_num, 0);

        // 5. WRITE DATA TO THE INODE
        printf("\n--- 5. Data Write ---\n");
        printf("Writing %zu bytes of data...\n", CONTENT_LENGTH);
        ssize_t written_bytes = fs_write(&fs, inode_num, TEST_CONTENT, CONTENT_LENGTH, 0);
        check_result("fs_write", written_bytes, CONTENT_LENGTH);

        // 6. READ DATA FROM THE INODE
        printf("\n--- 6. Data Read & Verification ---\n");
        ssize_t read_bytes = fs_read(&fs, inode_num, read_buffer, CONTENT_LENGTH, 0);
        check_result("fs_read", read_bytes, CONTENT_LENGTH);

        // 7. VERIFY CONTENT
        if (strncmp(read_buffer, TEST_CONTENT, CONTENT_LENGTH) == 0) {
            printf("[SUCCESS] Content verification succeeded.\n");
            printf("  Original: '%s'\n", TEST_CONTENT);
            printf("  Read Back: '%s'\n", read_buffer);
        } else {
            fprintf(stderr, "[FAILURE] Content mismatch!\n");
            fprintf(stderr, "  Expected: '%s'\n", TEST_CONTENT);
            fprintf(stderr, "  Received: '%.*s'\n", (int)read_bytes, read_buffer);
            goto cleanup;
        }
        
        // 8. REMOVE/DELETE THE INODE (Cleanup)
        printf("\n--- 7. Cleanup ---\n");
        // Assuming fs_remove returns a boolean (or 0/1 for success)
        if (fs_remove(&fs, inode_num)) {
            printf("[SUCCESS] fs_remove (inode %zd)\n", inode_num);
        } else {
            // Since fs_remove is currently a stub returning 'false' in your fs.c,
            // we'll treat it as a warning if it fails, but continue cleanup.
            fprintf(stderr, "[WARNING] fs_remove failed or is not yet implemented.\n");
        }

    cleanup:
        // 9. UNMOUNT THE FILE SYSTEM
        printf("\n--- 8. Unmount & Close ---\n");
        if (disk != NULL && disk->mounted) {
            fs_unmount(&fs);
            printf("[SUCCESS] fs_unmount\n");
        }

        // 10. CLOSE THE DISK
        if (disk != NULL) {
            disk_close(disk);
            printf("[SUCCESS] disk_close\n");
        }

        printf("\n--- Test Finished ---\n");

        if (inode_num < 0 || written_bytes != CONTENT_LENGTH || read_bytes != CONTENT_LENGTH || strncmp(read_buffer, TEST_CONTENT, CONTENT_LENGTH) != 0) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }