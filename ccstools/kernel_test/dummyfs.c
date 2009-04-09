/* This dummy filesystem is for 2.6.29. Build with "obj-y += dummyfs.o" */
#include <linux/fs.h>
#include <linux/init.h>

static int dummy_get_sb(struct file_system_type *fs_type,
			int flags, const char *dev_name, void *data,
			struct vfsmount *mnt)
{
	return -ENOMEM;
}

static struct file_system_type dummy_fs_type = {
	.name       = "fs\tname",
	.get_sb     = dummy_get_sb,
	.kill_sb    = kill_litter_super,
};

static int __init dummyfs_init(void)
{
	return register_filesystem(&dummy_fs_type);
}

__initcall(dummyfs_init);
