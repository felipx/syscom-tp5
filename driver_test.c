#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("/dev/7segs", O_RDWR);

    for (char i = 0x30; i < 0x40; i++) {
        write(fd, &i, 1);
        sleep(1);
    }

    close(fd);
}