#include "vfs.h"

static struct fuse_operations my_ops = {};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &my_ops, NULL);
}
