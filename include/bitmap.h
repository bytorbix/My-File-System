    #ifndef BITMAP_H
    #define BITMAP_H


    typedef struct Bitmap Bitmap;
        struct Bitmap {
            uint32_t *bitmap;   // Bitmap array Cache
        };

    bool format_bitmap(Disk *disk, uint32_t inode_blocks, uint32_t bitmap_blocks);
    bool save_bitmap(FileSystem *fs);
    bool load_bitmap(FileSystem *fs);


    #endif // FS_H