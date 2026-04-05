#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/buffer_head.h>

#include "predictfs.h"


#define MAGIC_NUMBER (0xf0f03410)

static const struct super_operations predictfs_super_ops = {
    
};

static int predictfs_fill_super(struct super_block *sb, struct fs_context *fc) {
    // initialize superblock fields
    if (sb_set_blocksize(sb, BLOCK_SIZE) == 0) {
        return -1; //error
    }
    sb->s_magic = MAGIC_NUMBER;
    sb->s_op = &predictfs_super_ops;
    
    
    struct buffer_head *bh = sb_bread(sb, 0);
    if (!bh) return -EIO;

    SuperBlock *disk_sb = (SuperBlock *)bh->b_data;
    // magic verify
    if (disk_sb->magic_number != MAGIC_NUMBER) {
        brelse(bh);
        return EINVAL;
    }
    brelse(bh);
    return 0;
};

static int predictfs_get_tree(struct fs_context *fc) {
    return get_tree_bdev(fc, predictfs_fill_super);
}


static const struct fs_context_operations predictfs_context_ops = {
    .get_tree = predictfs_get_tree,
};

int predictfs_init_fs_context(struct fs_context *fc) {
    fc->ops = &predictfs_context_ops;
    return 0;
}



static struct file_system_type fs = {
    .name = "predictfs",
    .init_fs_context = predictfs_init_fs_context,
    .kill_sb = kill_block_super,
    .fs_flags = FS_REQUIRES_DEV,
};


static int __init pfs_init(void) {
    register_filesystem(&fs);
    printk(KERN_INFO "predictfs: loaded!\n");
    return 0;
}

static void __exit pfs_exit(void) {
    unregister_filesystem(&fs);
    printk(KERN_INFO "predictfs: exiting!\n");
    return;
}

module_init(pfs_init);
module_exit(pfs_exit);
MODULE_LICENSE("GPL");