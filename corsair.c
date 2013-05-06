#define FUSE_USE_VERSION 26

#include <fuse/fuse.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <libtorrent.h>
#include <syslog.h>

#include "bdecode.h"

#define COR_DATA ((struct cor_state*) fuse_get_context()->private_data)

struct cor_state
{
	char* root;
	char* torrent;
	void* session;
};

static char const* priority[] =
{
		"EMERG:",
		"ALERT:",
		"CRIT:",
		"ERR:",
		"WARNING:",
		"NOTICE:",
		"INFO:",
};

// Helper Functions
void cor_usage()
{
	printf("Do things to the thing and you get the thing.");
}

static void cor_expand_path(char epath[PATH_MAX], const char* path)
{
	strcpy(epath, COR_DATA);
	strncat(epath, path, PATH_MAX);
}



// FUSE Operations
static int cor_getattr(const char* path, struct stat* stbuf)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_getattr");
	cor_expand_path(fpath, path);
	stat = lstat(fpath, stbuf);

	return stat;
}
static int cor_readlink(const char* path, char* link, size_t size)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_readlink");
	cor_expand_path(fpath, path);
	stat = readlink(fpath, link, size - 1);

	return stat;
}
static int cor_mknod(const char* path, mode_t mode, dev_t dev)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_mknod");
	cor_expand_path(fpath, path);

	if(S_ISREG(mode))
	{
		stat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if(stat < 0)
			fprintf(stderr, "Failed to create node %s.\n", path);
		else
		{
			stat = close(stat);
			if(stat < 0)
				fprintf(stderr, "Failed to drop node reference %s.\n", path);
		}
	}
	else
	{
		fprintf(stderr, "Attempted to create node of unsupported type %d.\n", mode);
	}
	return stat;
}
static int cor_mkdir(const char* path, mode_t mode)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_mkdir");
	cor_expand_path(fpath, path);

	stat = mkdir(fpath, mode);
	if(stat < 0)
		fprintf(stderr, "Failed to make directory %s.\n", path);

	return stat;
}
static int cor_unlink(const char* path)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_unlink");
	cor_expand_path(fpath, path);

	stat = unlink(fpath);
	if(stat < 0)
		fprintf(stderr, "Failed to unlink file %s.\n", path);

	return stat;
}
static int cor_rmdir(const char* path)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_rmdir");
	cor_expand_path(fpath, path);

	stat = rmdir(fpath);
	if(stat < 0)
		fprintf(stderr, "Failed to remove directory %s.\n", path);

	return stat;
}
static int cor_symlink(const char* path, const char* link)
{
	int stat = 0;
	char flink[PATH_MAX];

	fprintf(stderr, "cor_symlink");
	cor_expand_path(flink, link);

	stat = symlink(path, flink);
	if(stat < 0)
		fprintf(stderr, "Failed to create symbolic link %s.\n", path);

	return stat;
}
static int cor_rename(const char* path, const char* new)
{
	int stat = 0;
	char fpath[PATH_MAX];
	char fnew[PATH_MAX];

	fprintf(stderr, "cor_rename");
	cor_expand_path(fpath, path);
	cor_expand_path(fnew, new);

	stat = rename(fpath, fnew);
	if(stat < 0)
		fprintf(stderr, "Failed to rename %s.\n", path);

	return stat;
}
static int cor_link(const char* path, const char* new)
{
	int stat = 0;
	char fpath[PATH_MAX];
	char fnew[PATH_MAX];

	fprintf(stderr, "cor_link");
	cor_expand_path(fpath, path);
	cor_expand_path(fnew, new);

	stat = link(fpath, fnew);
	if(stat < 0)
		fprintf(stderr, "Failed to create hard link to %s.\n", new);

	return stat;
}
static int cor_chmod(const char* path, mode_t mode)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_chmod");
	stat = chmod(fpath, mode);
	if(stat < 0)
		fprintf(stderr, "Failed to change file mode for %s.\n", path);

	return stat;
}
static int cor_truncate(const char* path, off_t size)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_truncate");
	cor_expand_path(fpath, path);

	stat = truncate(fpath, size);
	if(stat < 0)
		fprintf(stderr, "Failed to resize file %s.\n", path);

	return stat;
}
static int cor_utime(const char* path, struct utimbuf* ubuf)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_utime");
	cor_expand_path(fpath, path);

	stat = utime(fpath, ubuf);
	if(stat < 0)
		fprintf(stderr, "Could not change time attributes of %s.\n", path);

	return stat;
}
// TODO: Swap out the integer file-descriptor for our more useful
// struct that keeps track of the file's download progress.
static int cor_open(const char* path, struct fuse_file_info* fi)
{
	int fd;
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_open");
	cor_expand_path(fpath, path);

	fd = open(fpath, fi->flags);
	if(fd < 0)
		fprintf(stderr, "Could not open file %s.\n", path);

	fi->fh = fd;

	return stat;
}
static int cor_read(const char* path, char* rbuf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	int stat = 0;

	fprintf(stderr, "cor_read");
	stat = pread(fi->fh, rbuf, size, offset);
	if(stat < 0)
		fprintf(stderr, "Failed to read from file %s.\n", path);

	return stat;
}
static int cor_write(const char* path, char* wbuf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	int stat = 0;

	fprintf(stderr, "cor_write");
	stat = pwrite(fi->fh, wbuf, size, offset);
	if(stat < 0)
		fprintf(stderr, "Failed to write to file %s.\n", path);

	return stat;
}
static int cor_statfs(const char* path, struct statvfs* statv)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_statfs");
	cor_expand_path(fpath, path);

	stat = statvfs(fpath, statv);
	if(stat < 0)
		fprintf(stderr, "Failed to stat %s.\n", path);

	return stat;
}
static int cor_flush(const char* path, struct fuse_file_info* fi)
{
	(void) path;
	(void) fi;

	fprintf(stderr, "cor_flush");

	return 0;
}
static int cor_release(const char* path, struct fuse_file_info* fi)
{
	int stat = 0;

	fprintf(stderr, "cor_release");
	stat = close(fi->fh);
	return stat;
}
static int cor_fsync(const char* path, int datasync, struct fuse_file_info* fi)
{
	int stat = 0;

	fprintf(stderr, "cor_fsync");
	if(datasync)
		stat = fdatasync(fi->fh);
	else
		stat = fsync(fi->fh);

	if(stat < 0)
		fprintf(stderr, "Failed to sync data for %s.\n", path);

	return stat;
}
static int cor_setxattr(const char* path, const char* name, const char* value, size_t size, int flags)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_setxattr");
	cor_expand_path(fpath, path);

	stat = lsetxattr(fpath, name, value, size, flags);
	if(stat < 0)
		fprintf(stderr, "Could not set extended attribute of %s.\n", path);

	return stat;
}
static int cor_getxattr(const char* path, const char* name, char* value, size_t size)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_getxattr");
	cor_expand_path(fpath, path);

	stat = lgetxattr(fpath, name, value, size);
	if(stat < 0)
		fprintf(stderr, "Could not get extended attribute of %s.\n", path);

	return stat;
}
static int cor_listxattr(const char* path, char* list, size_t size)
{
	int stat = 0;
	char fpath[PATH_MAX];
	char* atptr;

	fprintf(stderr, "cor_listxattr");
	cor_expand_path(fpath, path);

	stat = llistxattr(fpath, list, size);
	if(stat < 0)
		fprintf(stderr, "Failed to list extended attributes for %s.\n", path);

	return stat;
}
static int cor_removexattr(const char* path, const char* name)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_removexattr");
	cor_expand_path(fpath, path);

	stat = lremovexattr(fpath, name);
	if(stat < 0)
		fprintf(stderr, "Failed to remove extended attribute from %s.\n", path);

	return stat;
}
static int cor_opendir(const char* path, struct fuse_file_info* fi)
{
	DIR* dp;
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_opendir");
	cor_expand_path(fpath, path);

	dp = opendir(fpath);
	if(dp == NULL)
		fprintf(stderr, "Could not open directory %s.\n", path);

	fi->fh = (intptr_t)dp;

	return stat;
}
static int cor_readdir(const char* path, void* rdbuf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
	DIR* dp;
	int stat = 0;
	struct dirent* de;

	fprintf(stderr, "cor_readdir");
	dp = (DIR*)(uintptr_t)fi->fh;

	de = readdir(dp);
	if(de == 0)
	{
		fprintf(stderr, "Failed to read directory entry for %s.\n", path);
		return stat;
	}

	do
	{
		if(filler(rdbuf, de->d_name, NULL, 0) != 0)
		{
			fprintf(stderr, "Filler couldn't complete task due to buffer overflow.\n");
			return -ENOMEM;
		}
	} while((de == readdir(dp)) != NULL);

	return stat;
}
static int cor_releasedir(const char* path, struct fuse_file_info* fi)
{
	int stat = 0;

	fprintf(stderr, "cor_releasedir");
	closedir((DIR*)(uintptr_t)fi->fh);
	return stat;
}
static int cor_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi)
{
	(void) path;
	(void) datasync;
	(void) fi;

	fprintf(stderr, "cor_fsyncdir");
	return 0;
}
static void* cor_init(struct fuse_conn_info* ci)
{
	printf("Mounting %s as volume...\n", COR_DATA->torrent);

	FILE* inf;
	char* fbuf;
	long flen;

	fprintf(stderr, "cor_init");
	// Attempt to open torrent file for reading.
	inf = fopen(COR_DATA->torrent, "r");
	if(inf == NULL)
		return 1;

	// Seek to end of file to determine its length.
	fseek(inf, 0L, SEEK_END);
	flen = ftell(inf);
	fseek(inf, 0L, SEEK_SET);

	// Allocate buffer of appropriate size.
	fbuf = (char*)calloc(flen, sizeof(char));
	if(fbuf == NULL)
		return 1;

	fread(fbuf, sizeof(char), flen, inf);
	fclose(inf);

	// Decode the bencoded torrent description.
	bd_dict* file_dict = decode(fbuf, flen);

	// Free file buffer.
	free(fbuf);

	// Create session.
	COR_DATA->session = session_create
	(
		SES_FINGERPRINT, 		"CS",
		SES_LISTENPORT, 		6881,
		SES_LISTENPORT_END, 	6889,
		SES_VERSION_MAJOR, 		1,
		SES_VERSION_MINOR, 		1,
		SES_VERSION_TINY, 		1,
		SES_VERSION_TAG, 			42,
		TAG_END
	);

	// Add the mounted torrent to the session.
	int tnum = session_add_torrent
	(
		COR_DATA->session,
		TOR_FILENAME, COR_DATA->torrent,
		TOR_SAVE_PATH, COR_DATA->root,
		TAG_END
	);

	printf("Mounting to %s.\n", COR_DATA->root);

	return COR_DATA;
}
// TODO: Save session resume data and possibly cache
// entire volume to a .torrent and submit to a tracker?
static void cor_destroy(void* userdata)
{
	fprintf(stderr, "cor_destroy");
	(void) userdata;
}
static int cor_access(const char* path, int mask)
{
	int stat = 0;
	char fpath[PATH_MAX];

	fprintf(stderr, "cor_access");
	cor_expand_path(fpath, path);

	stat = access(fpath, mask);
	if(stat < 0)
		fprintf(stderr, "Could not grant access to %s.\n", stat);

	return stat;
}
static int cor_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
	int stat = 0;
	char fpath[PATH_MAX];
	int fd;

	fprintf(stderr, "cor_create");
	cor_expand_path(fpath, path);

	fd = creat(fpath, mode);
	if(fd < 0)
		fprintf(stderr, "Could not create file %s.\n", path);

	fi->fh = fd;

	return stat;
}
static int cor_ftruncate(const char* path, off_t offset, struct fuse_file_info* fi)
{
	int stat = 0;

	fprintf(stderr, "cor_ftruncate");
	stat = ftruncate(fi->fh, offset);
	if(stat < 0)
		fprintf(stderr, "Failed to resize file %s.\n", path);

	return stat;
}
static int cor_fgetattr(const char* path, struct stat* statbuf, struct fuse_file_info *fi)
{
	int stat = 0;

	fprintf(stderr, "cor_fgetattr");
	stat = fstat(fi->fh, statbuf);
	if(stat < 0)
		fprintf(stderr, "Failed to get attributes for file %s.\n", path);

	return stat;
}

// Struct binding implemented funcs.
static struct fuse_operations cor_ops =
{
  .getattr = cor_getattr,
  .readlink = cor_readlink,
  .mknod = cor_mknod,
  .mkdir = cor_mkdir,
  .unlink = cor_unlink,
  .rmdir = cor_rmdir,
  .symlink = cor_symlink,
  .rename = cor_rename,
  .link = cor_link,
  .chmod = cor_chmod,
  .truncate = cor_truncate,
  .utime = cor_utime,
  .open = cor_open,
  .read = cor_read,
  .write = cor_write,
  .statfs = cor_statfs,
  .flush = cor_flush,
  .release = cor_release,
  .fsync = cor_fsync,
  .setxattr = cor_setxattr,
  .getxattr = cor_getxattr,
  .listxattr = cor_listxattr,
  .removexattr = cor_removexattr,
  .opendir = cor_opendir,
  .readdir = cor_readdir,
  .releasedir = cor_releasedir,
  .fsyncdir = cor_fsyncdir,
  .init = cor_init,
  .destroy = cor_destroy,
  .access = cor_access,
  .create = cor_create,
  .ftruncate = cor_ftruncate,
  .fgetattr = cor_fgetattr,
};

int main(int argc, char* argv[])
{
  // Init empty arguments list.
  struct cor_state* state;
  struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

  // Make sure executing user isn't being an idiot.
  if((getuid() == 0) || (geteuid() == 0))
  {
	  fprintf(stderr, "Mounting a CorsairFS volume as root "
			  	  	  "poses a massive security vulnerability.\n");
	  return 1;
  }

  // Make sure our passed arguments are good.
  if(argc < 3)
  {
    cor_usage();
    return 1;
  }

  state = malloc(sizeof(struct cor_state));
  state->root = realpath(argv[1], NULL);
  state->torrent = realpath(argv[2], NULL);

  fuse_opt_add_arg(&args, argv[0]);
  fuse_opt_add_arg(&args, argv[1]);

  int x;
  x = fuse_main(args.argc, args.argv, &cor_ops, state);

  printf("%d", x);

  return 0;
}
