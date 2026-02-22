#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"
#include "dir.h"

void print_passed(const char* message) { printf("[OK]:   %s\n", message); }
void print_failed(const char* message) { printf("[FAIL]: %s\n", message); }

int main() {
    Disk *disk = disk_open("mfs_test_disk.img", 100);
    fs_format(disk);

    FileSystem fs = {0};
    fs_mount(&fs, disk);

    ssize_t inode_file1 = fs_create(&fs);
    ssize_t inode_file2 = fs_create(&fs);
    ssize_t inode_dir1  = dir_create(&fs);

    // Test 1: dir_create
    printf("\n--- Test 1: dir_create ---\n");
    if (inode_dir1 >= 0) print_passed("dir_create returned a valid inode");
    else                 print_failed("dir_create failed");

    // Test 2: dir_add + dir_lookup
    printf("\n--- Test 2: dir_add + dir_lookup ---\n");
    if (dir_add(&fs, inode_dir1, "file1", inode_file1) == 0)
        print_passed("dir_add: 'file1' added");
    else
        print_failed("dir_add: 'file1' failed");

    if (dir_lookup(&fs, inode_dir1, "file1") == inode_file1)
        print_passed("dir_lookup: 'file1' found with correct inode");
    else
        print_failed("dir_lookup: 'file1' not found or wrong inode");

    // Test 3: lookup a name that doesn't exist
    printf("\n--- Test 3: dir_lookup non-existent ---\n");
    if (dir_lookup(&fs, inode_dir1, "ghost") == -1)
        print_passed("dir_lookup: 'ghost' correctly returned -1");
    else
        print_failed("dir_lookup: 'ghost' should return -1");

    // Test 4: duplicate name rejected
    printf("\n--- Test 4: duplicate dir_add ---\n");
    if (dir_add(&fs, inode_dir1, "file1", inode_file2) == -1)
        print_passed("dir_add: duplicate correctly rejected");
    else
        print_failed("dir_add: duplicate should be rejected");

    // Test 5: dir_remove
    printf("\n--- Test 5: dir_remove ---\n");
    if (dir_remove(&fs, inode_dir1, "file1") == inode_file1)
        print_passed("dir_remove: returned correct inode");
    else
        print_failed("dir_remove: failed or wrong inode");

    if (dir_lookup(&fs, inode_dir1, "file1") == -1)
        print_passed("dir_lookup after remove: 'file1' is gone");
    else
        print_failed("dir_lookup after remove: 'file1' should not exist");

    fs_unmount(&fs);
    return 0;
}
