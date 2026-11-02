/*
 * td_py_vfs.c - the TinyDesk file system for MicroPython.
 *
 * A VFS object mounted at "/" whose paths are TinyDesk paths (/home/root,
 * /etc, ...). Each one goes through TinyDesk Shell's tdsh_path_to_real(),
 * the function the shell's own commands use, so a script reaches the same
 * files as `cat` or `ls` and cannot step outside the TinyDesk root with
 * "..". Open files are the POSIX file objects of extmod/vfs_posix_file.c.
 *
 * The type is MicroPython's mp_type_vfs_posix (os.VfsPosix): this build
 * has no other VfsPosix, so a script cannot make one rooted elsewhere.
 */
#include "td_py_vfs.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "extmod/vfs.h"
#include "extmod/vfs_posix.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "td_py.h"

#ifndef O_BINARY
#define O_BINARY 0   /* only Windows translates line ends */
#endif

typedef struct
{
    mp_obj_base_t base;
    char cwd[TDSH_MAX_PATH];   /* a TinyDesk path */
} td_vfs_t;

/* path (absolute, or relative to self->cwd) -> real path in real[]. */
static int resolve(td_vfs_t *self, const char *path, char *real, size_t cap, char *logical, size_t lcap)
{
    tdsh_session_t *session = td_py_session();
    if (!session)
        return -EIO;
    char joined[TDSH_MAX_PATH * 2];
    int n = path[0] == '/' ? snprintf(joined, sizeof(joined), "%s", path)
                           : snprintf(joined, sizeof(joined), "%s/%s", self->cwd, path);
    if (n < 0 || (size_t)n >= sizeof(joined))
        return -ENAMETOOLONG;
    return tdsh_path_to_real(session, joined, real, cap, logical, lcap);
}

static void resolve_or_raise(td_vfs_t *self, mp_obj_t path_in, char *real, size_t cap, char *logical, size_t lcap)
{
    int rc = resolve(self, mp_obj_str_get_str(path_in), real, cap, logical, lcap);
    if (rc)
        mp_raise_OSError(-rc);
}

static void check(int rc)
{
    if (rc != 0)
        mp_raise_OSError(errno);
}

/* -------------------------------------------------------- methods */

static mp_obj_t td_vfs_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    (void)n_args;
    (void)n_kw;
    (void)args;
    td_vfs_t *self = mp_obj_malloc(td_vfs_t, type);
    strcpy(self->cwd, "/");
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t td_vfs_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs)
{
    (void)self_in;
    (void)readonly;
    if (mp_obj_is_true(mkfs))
        mp_raise_OSError(MP_EPERM);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(td_vfs_mount_obj, td_vfs_mount);

static mp_obj_t td_vfs_umount(mp_obj_t self_in)
{
    (void)self_in;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(td_vfs_umount_obj, td_vfs_umount);

/* Files are opened here, always in binary mode (TinyDesk files use "\n"
 * on every platform; Windows would write "\r\n" in text mode), and the
 * descriptor is handed to vfs_posix_file.c's file object. */
static mp_obj_t td_vfs_open(mp_obj_t self_in, mp_obj_t path_in, mp_obj_t mode_in)
{
    char real[TDSH_MAX_REAL_PATH];
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), path_in, real, sizeof(real), NULL, 0);
    const char *mode = mp_obj_str_get_str(mode_in);
    int rw = O_RDONLY, extra = 0;
    for (const char *m = mode; *m; m++)
    {
        if (*m == 'w')
        {
            rw = O_WRONLY;
            extra = O_CREAT | O_TRUNC;
        }
        else if (*m == 'a')
        {
            rw = O_WRONLY;
            extra = O_CREAT | O_APPEND;
        }
        else if (*m == '+')
        {
            rw = O_RDWR;
        }
    }
    int fd = open(real, rw | extra | O_BINARY, 0644);
    if (fd < 0)
        mp_raise_OSError(errno);
    const mp_obj_type_t *type = strchr(mode, 'b') ? &mp_type_vfs_posix_fileio : &mp_type_vfs_posix_textio;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0)
    {
        mp_obj_t file = mp_vfs_posix_file_open(type, MP_OBJ_NEW_SMALL_INT(fd), mode_in);
        nlr_pop();
        return file;
    }
    close(fd);   /* no memory for the file object */
    nlr_jump(nlr.ret_val);
}
static MP_DEFINE_CONST_FUN_OBJ_3(td_vfs_open_obj, td_vfs_open);

static mp_obj_t td_vfs_chdir(mp_obj_t self_in, mp_obj_t path_in)
{
    td_vfs_t *self = MP_OBJ_TO_PTR(self_in);
    char real[TDSH_MAX_REAL_PATH];
    char logical[TDSH_MAX_PATH];
    resolve_or_raise(self, path_in, real, sizeof(real), logical, sizeof(logical));
    struct stat st;
    check(stat(real, &st));
    if (!S_ISDIR(st.st_mode))
        mp_raise_OSError(MP_ENOTDIR);
    strcpy(self->cwd, logical);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(td_vfs_chdir_obj, td_vfs_chdir);

static mp_obj_t td_vfs_getcwd(mp_obj_t self_in)
{
    td_vfs_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_str(self->cwd, strlen(self->cwd));
}
static MP_DEFINE_CONST_FUN_OBJ_1(td_vfs_getcwd_obj, td_vfs_getcwd);

/* ilistdir(path): an iterator of (name, type, inode) tuples. The listing
 * is read at once, so no directory handle is left open if the script
 * stops iterating early. */
static mp_obj_t td_vfs_ilistdir(mp_obj_t self_in, mp_obj_t path_in)
{
    char real[TDSH_MAX_REAL_PATH];
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), path_in, real, sizeof(real), NULL, 0);
    DIR *dir = opendir(real);
    if (!dir)
        mp_raise_OSError(errno);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0)
    {
        size_t base = strlen(real);
        struct dirent *de;
        while ((de = readdir(dir)) != NULL)
        {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;
            mp_obj_t t[3];
            t[0] = mp_obj_new_str(de->d_name, strlen(de->d_name));
            t[1] = MP_OBJ_NEW_SMALL_INT(0);
            t[2] = MP_OBJ_NEW_SMALL_INT(0);
            if (base + 1 + strlen(de->d_name) < sizeof(real))
            {
                snprintf(real + base, sizeof(real) - base, "/%s", de->d_name);
                struct stat st;
                if (stat(real, &st) == 0)
                    t[1] = MP_OBJ_NEW_SMALL_INT(S_ISDIR(st.st_mode) ? MP_S_IFDIR : MP_S_IFREG);
                real[base] = '\0';
            }
            mp_obj_list_append(list, mp_obj_new_tuple(3, t));
        }
        nlr_pop();
        closedir(dir);
    }
    else
    {
        closedir(dir);
        nlr_jump(nlr.ret_val);
    }
    return mp_getiter(list, NULL);
}
static MP_DEFINE_CONST_FUN_OBJ_2(td_vfs_ilistdir_obj, td_vfs_ilistdir);

static mp_obj_t td_vfs_mkdir(mp_obj_t self_in, mp_obj_t path_in)
{
    char real[TDSH_MAX_REAL_PATH];
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), path_in, real, sizeof(real), NULL, 0);
#if defined(_WIN32)
    check(mkdir(real));
#else
    check(mkdir(real, 0777));
#endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(td_vfs_mkdir_obj, td_vfs_mkdir);

static mp_obj_t td_vfs_remove(mp_obj_t self_in, mp_obj_t path_in)
{
    char real[TDSH_MAX_REAL_PATH];
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), path_in, real, sizeof(real), NULL, 0);
    check(unlink(real));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(td_vfs_remove_obj, td_vfs_remove);

static mp_obj_t td_vfs_rmdir(mp_obj_t self_in, mp_obj_t path_in)
{
    char real[TDSH_MAX_REAL_PATH];
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), path_in, real, sizeof(real), NULL, 0);
    check(rmdir(real));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(td_vfs_rmdir_obj, td_vfs_rmdir);

static mp_obj_t td_vfs_rename(mp_obj_t self_in, mp_obj_t old_in, mp_obj_t new_in)
{
    char from[TDSH_MAX_REAL_PATH];
    char to[TDSH_MAX_REAL_PATH];
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), old_in, from, sizeof(from), NULL, 0);
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), new_in, to, sizeof(to), NULL, 0);
    check(rename(from, to));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(td_vfs_rename_obj, td_vfs_rename);

static mp_obj_t td_vfs_stat(mp_obj_t self_in, mp_obj_t path_in)
{
    char real[TDSH_MAX_REAL_PATH];
    resolve_or_raise(MP_OBJ_TO_PTR(self_in), path_in, real, sizeof(real), NULL, 0);
    struct stat st;
    check(stat(real, &st));
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(S_ISDIR(st.st_mode) ? MP_S_IFDIR : MP_S_IFREG);
    t->items[1] = MP_OBJ_NEW_SMALL_INT(0);
    t->items[2] = MP_OBJ_NEW_SMALL_INT(0);
    t->items[3] = MP_OBJ_NEW_SMALL_INT(0);
    t->items[4] = MP_OBJ_NEW_SMALL_INT(0);
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0);
    t->items[6] = mp_obj_new_int_from_uint((mp_uint_t)st.st_size);
    t->items[7] = mp_obj_new_int_from_ll((long long)st.st_atime);
    t->items[8] = mp_obj_new_int_from_ll((long long)st.st_mtime);
    t->items[9] = mp_obj_new_int_from_ll((long long)st.st_ctime);
    return MP_OBJ_FROM_PTR(t);
}
static MP_DEFINE_CONST_FUN_OBJ_2(td_vfs_stat_obj, td_vfs_stat);

/* For import: is path a file, a directory, or nothing? Must not raise. */
static mp_import_stat_t td_vfs_import_stat(void *self_in, const char *path)
{
    char real[TDSH_MAX_REAL_PATH];
    if (resolve(self_in, path, real, sizeof(real), NULL, 0) != 0)
        return MP_IMPORT_STAT_NO_EXIST;
    struct stat st;
    if (stat(real, &st) != 0)
        return MP_IMPORT_STAT_NO_EXIST;
    return S_ISDIR(st.st_mode) ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_FILE;
}

static const mp_rom_map_elem_t td_vfs_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&td_vfs_mount_obj)},
    {MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&td_vfs_umount_obj)},
    {MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&td_vfs_open_obj)},
    {MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&td_vfs_chdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&td_vfs_getcwd_obj)},
    {MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&td_vfs_ilistdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&td_vfs_mkdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&td_vfs_remove_obj)},
    {MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&td_vfs_rmdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&td_vfs_rename_obj)},
    {MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&td_vfs_stat_obj)},
};
static MP_DEFINE_CONST_DICT(td_vfs_locals_dict, td_vfs_locals_table);

static const mp_vfs_proto_t td_vfs_proto = {
    .import_stat = td_vfs_import_stat,
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_vfs_posix,
    MP_QSTR_VfsTinyDesk,
    MP_TYPE_FLAG_NONE,
    make_new, td_vfs_make_new,
    protocol, &td_vfs_proto,
    locals_dict, &td_vfs_locals_dict);

/* ------------------------------------------------------------ setup */

void td_py_vfs_mount_root(const char *cwd)
{
    mp_obj_t vfs = td_vfs_make_new(&mp_type_vfs_posix, 0, 0, NULL);
    mp_obj_t args[2] = {vfs, MP_OBJ_NEW_QSTR(MP_QSTR__slash_)};
    mp_vfs_mount(2, args, (mp_map_t *)&mp_const_empty_map);
    mp_vfs_chdir(mp_obj_new_str(cwd, strlen(cwd)));
}
