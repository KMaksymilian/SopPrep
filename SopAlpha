#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 1001
#define MAX_BUFF 1001
#define MAX_FD 20

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void print_file(const char* path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        ERR("open");
    }
    printf("File contents:\n");
    char buff[MAX_BUFF + 1];
    ssize_t ret = bulk_read(fd, buff, MAX_BUFF);
    while (ret > 0)
    {
        buff[ret] = '\0';
        printf("%s", buff);
        ret = bulk_read(fd, buff, MAX_BUFF);
    }
    printf("\n");
    if (ret < 0)
    {
        ERR("bulk_read");
    }

    if (close(fd))
    {
        ERR("close");
    }
}

void print_dir(const char* path)
{
    char last_path[MAX_PATH];
    if (getcwd(last_path, MAX_PATH) == NULL)
    {
        ERR("cwd");
    }
    if (chdir(path))
    {
        ERR("chdir");
    }
    DIR* dirp;
    struct dirent* dp;
    struct stat filestat;

    dirp = opendir(".");
    if (dirp == NULL)
    {
        ERR("opendir");
    }

    do
    {
        errno = 0;
        dp = readdir(dirp);
        if (dp == NULL)
        {
            break;
        }

        if (stat(dp->d_name, &filestat))
        {
            ERR("stat");
        }
        printf("%s\n", dp->d_name);
    } while (dp != NULL);

    if (errno != 0)
    {
        ERR("readdir");
    }

    if (closedir(dirp))
    {
        ERR("closedir");
    }

    if (chdir(last_path))
    {
        ERR("chdir");
    }
}

void show_stage2(const char* const path, const struct stat* const stat_buf)
{
    if (S_ISREG(stat_buf->st_mode))
    {
        printf("File size: %ld\n", stat_buf->st_size);
        print_file(path);
    }
    else if (S_ISDIR(stat_buf->st_mode))
    {
        print_dir(path);
    }
    else
    {
        printf("Unknown file type.\n");
    }
}

void write_stage3(const char* const path, const struct stat* const stat_buf)
{
    (void)stat_buf;

    int fd = open(path, O_APPEND | O_WRONLY);
    if (fd < 0)
    {
        ERR("open");
    }

    size_t count;
    while (1)
    {
        char buff[MAX_BUFF];
        if (fgets(buff, MAX_BUFF, stdin) == NULL)
        {
            ERR("fgets");
        }
        count = strlen(buff);
        if (count < 2)
        {
            break;
        }
        buff[count] = '\0';

        bulk_write(fd, buff, count);
    }

    if (close(fd))
    {
        ERR("close");
    }
}

int walk(const char* name, const struct stat* filestat, int type, struct FTW* f)
{
    (void)type;
    (void)f;
    printf("%s -- ", name);
    if (S_ISREG(filestat->st_mode))
    {
        printf("regular\n");
    }
    else if (S_ISDIR(filestat->st_mode))
    {
        printf("directory\n");
    }
    else
    {
        printf("other\n");
    }
    return 0;
}

void walk_stage4(const char *const path, const struct stat *const stat_buf)
{
    (void)stat_buf;
    if (nftw(path, walk, MAX_FD, FTW_PHYS) != 0)
    {
        printf("err?");
    }
}

int check_path(char* buff, struct stat* filestat)
{
    if (fgets(buff, MAX_PATH, stdin) == NULL)
    {
        ERR("fgets");
    }
    buff[strlen(buff) - 1] = '\0';

    if (stat(buff, filestat))
    {
        if (errno != ENOENT)
        {
            ERR("stat");
        }
        printf("File does not exist!\n");
        return 1;
    }
    return 0;
}

int interface_stage1()
{
    printf("1. show\n2. write\n3. walk\n4. exit\n");

    char buff[MAX_PATH];
    if (fgets(buff, MAX_PATH, stdin) == NULL)
    {
        ERR("fgets");
    }
    if (strlen(buff) > 2)
    {
        printf("Incorrect operation.\n");
        return 1;
    }

    struct stat filestat;

    switch (buff[0])
    {
        case '1':
            if (check_path(buff, &filestat))
            {
                return 1;
            }
            show_stage2(buff, &filestat);
            return 1;
        case '2':
            if (check_path(buff, &filestat))
            {
                return 1;
            }
            write_stage3(buff, &filestat);
            return 1;
        case '3':
            if (check_path(buff, &filestat))
            {
                return 1;
            }
            walk_stage4(buff, &filestat);
            return 1;
        case '4':
            return 0;
        default:
            printf("Incorrect operation.\n");
            return 1;
    }

    return 0;
}

int main()
{
    while (interface_stage1())
        ;
    return EXIT_SUCCESS;
}
