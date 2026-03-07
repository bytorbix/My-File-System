#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "disk.h"
#include "fs.h"
#include "dir.h"
#include "utils.h"
#include "pfs.h"

void print_passed(const char* message) { printf("[OK]:   %s\n", message); }
void print_failed(const char* message) { printf("[FAIL]: %s\n", message); }

void bitmap_stats(FileSystem *fs)
{
    uint32_t total = fs->meta_data->blocks;
    uint32_t used = 0;
    for (uint32_t i = 0; i < total; i++)
        if (get_bit(fs->bitmap->bits, i)) used++;

    printf("bitmap: %u/%u blocks used, %u free (%u bitmap block(s))\n",
           used, total, total - used, fs->meta_data->bitmap_blocks);
}

void ls(FileSystem *fs, ssize_t dir_inode) // Demo ls command for test case
{
    // Validation check
    if (fs == NULL || fs->disk == NULL) {
        perror("ls: Error fs or disk is invalid (NULL)");
        return;
    }
    if (!fs->disk->mounted)
    {
        fprintf(stderr, "ls: Error disk is not mounted, cannot procceed t\n");
        return;
    }

    // Read the directory inode and confirm it's a directory
    uint32_t inode_block_idx = 1 + (dir_inode / INODES_PER_BLOCK);
    uint32_t inode_offset = dir_inode % INODES_PER_BLOCK;

    Block inode_buf;
    if (disk_read(fs->disk, inode_block_idx, inode_buf.data) < 0) return;

    Inode *target = &inode_buf.inodes[inode_offset]; // our dir inode

    if (target->valid != INODE_DIR)
    {
        perror("ls: Inode given is not a directory.");
        return;
    }
    int count = 0;
    for (size_t i = 0; i < target->size; i+=32) 
    {
        DirEntry entry;
        if (fs_read(fs, dir_inode, (char *)&entry, sizeof(DirEntry), i) < 0) return;
        if ((entry.inode_number != UINT32_MAX)) 
        {
            size_t file_size = fs_stat(fs, (size_t)entry.inode_number);
            printf("File: %d (Inode %d) - %s (Size %ld)\n", count, entry.inode_number, entry.name, file_size);
            count++;
        }
    }
}

void cat(FileSystem *fs, ssize_t inode_file) 
{
    // Validation check
    if (fs == NULL || fs->disk == NULL) {
        perror("cat: Error fs or disk is invalid (NULL)");
        return;
    }
    if (!fs->disk->mounted)
    {
        fprintf(stderr, "cat: Error disk is not mounted, cannot procceed t\n");
        return;
    }

    size_t file_size = fs_stat(fs, inode_file);
    if (file_size <= 0) return;

    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        perror("cat: malloc failed");
        return;
    }
    if (fs_read(fs, inode_file, buffer, file_size, 0) < 0) 
    { 
        free(buffer);
        return; 
    }
    buffer[file_size] = '\0';

    int word_count = 0;
    for (size_t i = 0; i < file_size; i++) {
        putchar(buffer[i]);

        if (isspace(buffer[i])) {
            word_count++;

            if (word_count >= 30) {
                putchar('\n');
                word_count = 0;

                while (i + 1 < file_size && isspace(buffer[i + 1])) { // skip extra spaces
                    i++;
                }
            }
        }
    }
    printf("\n");
    free(buffer);
}


int main() {
    Disk *disk = disk_open("disk.img", 100000);
    pfs_format(disk);

    pFileSystem *pfs = calloc(1, sizeof(pFileSystem));
    pfs_mount(pfs, disk);

    // Allocate a directory
    ssize_t inode_dir1 = dir_create(pfs->fs);
    ssize_t inode_sub_dir1 = dir_create(pfs->fs);
    ssize_t inode_sub_dir2 = dir_create(pfs->fs);

    // Adding directories
    if (dir_add(pfs->fs, 0, "dir1", inode_dir1) < 0) {
        print_failed("dir_add test_case1 failed");
    }
    if (dir_add(pfs->fs, inode_dir1, "dir2", inode_sub_dir1) < 0) {
        print_failed("dir_add test_case2 failed");
    }
    if (dir_add(pfs->fs, inode_sub_dir1, "dir3", inode_sub_dir2) < 0) {
        print_failed("dir_add test_case3 failed");
    }

    // Adding the files 
    ssize_t inode = pfs_create(pfs, "/dir1/dir2/dir3/file1.txt");
    // Writing data into one of the files
    char *text = "Hello World!";
    if (pfs_write(pfs, inode, text, strlen(text), 0) < 0) return -1;

    // Lookup the sub directories that holds the files
    ssize_t desired_dir  = fs_lookup(pfs->fs, "/dir1/dir2/dir3");
    
    // List files in the directory
    ls(pfs->fs, desired_dir);

    // print the file with content
    cat(pfs->fs, inode);

    bitmap_stats(pfs->fs);

    // Double Indirect Test (5MB file, exceeds single indirect ~4MB range)
    size_t large_size = 5 * 1024 * 1024;
    char *write_buf = malloc(large_size);
    char *read_buf  = malloc(large_size);
    memset(write_buf, 0xAB, large_size);

    ssize_t inode_large = pfs_create(pfs, "/dir1/dir2/dir3/largefile.bin");
    pfs_write(pfs, inode_large, write_buf, large_size, 0);
    fs_read(pfs->fs, inode_large, read_buf, large_size, 0);

    if (memcmp(write_buf, read_buf, large_size) == 0)
        print_passed("double indirect: 5MB write/read verified");
    else
        print_failed("double indirect: data mismatch");

    bitmap_stats(pfs->fs);
    fs_remove(pfs->fs, inode_large);
    bitmap_stats(pfs->fs);

    free(write_buf);
    free(read_buf);
     

    // Close and exit
    pfs_unmount(pfs);
    free(pfs);
    return 0;
}

