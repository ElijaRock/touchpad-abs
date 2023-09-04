#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/uinput.h>
#include <sys/ioctl.h>

// These values should not be changed by the user
#define CODE_ID_BYTE 18

// The below values are device specific and should be changed by the user

// Running `evtest` as root user with no arguments
// will list the available devices. Find the "touchpad"
// one and store its number in the below constant
#define TOUCHPAD_EVENT_X "event11"

#define RES_WIDTH 1920
#define RES_HEIGHT 1080

// This is where the user needs to run `evtest` as root
// and move their finger to the bottom right corner
// of their touchpad to determine the maximum values
// Ie: `sudo evtest /dev/input/eventX |& grep ABS`
// where "eventX" is replaced by the event number of
// your touchpad. Eg: "event5"
#define MAX_ABS_X 3192
#define MAX_ABS_Y 1824

static void fatal(const char *msg) {
    fprintf(stderr, "fatal: ");

    if (errno)
        perror(msg);
    else
        fprintf(stderr, "%s\n", msg);

    exit(EXIT_FAILURE);
}

static void setup_abs(int fd, int type, int min, int max, int res) {
    struct uinput_abs_setup abs = {
        .code = type,
        .absinfo = {
            .minimum = min,
            .maximum = max,
            .resolution = res
        }
    };

    if (-1 == ioctl(fd, UI_ABS_SETUP, &abs))
        fatal("ioctl UI_ABS_SETUP");
}

static void init(int fd, int width, int height, int dpi) {
    if (-1 == ioctl(fd, UI_SET_EVBIT, EV_SYN))
        fatal("ioctl UI_SET_EVBIT EV_SYN");

    if (-1 == ioctl(fd, UI_SET_EVBIT, EV_KEY))
        fatal("ioctl UI_SET_EVBIT EV_KEY");
    if (-1 == ioctl(fd, UI_SET_KEYBIT, BTN_LEFT))
        fatal("ioctl UI_SET_KEYBIT BTN_LEFT");

    if (-1 == ioctl(fd, UI_SET_EVBIT, EV_ABS))
        fatal("ioctl UI_SET_EVBIT EV_ABS");
    /* the ioctl UI_ABS_SETUP enables these automatically, when appropriate:
        ioctl(fd, UI_SET_ABSBIT, ABS_X);
        ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    */

    struct uinput_setup device = {
        .id = {
            .bustype = BUS_USB
        },
        .name = "Emulated Absolute Positioning Device"
    };

    if (-1 == ioctl(fd, UI_DEV_SETUP, &device))
        fatal("ioctl UI_DEV_SETUP");

    setup_abs(fd, ABS_X, 0, width, dpi);
    setup_abs(fd, ABS_Y, 0, height, dpi);

    if (-1 == ioctl(fd, UI_DEV_CREATE))
        fatal("ioctl UI_DEV_CREATE");

    /* give time for device creation */
    sleep(1);
}

static void emit(int fd, int type, int code, int value) {
    struct input_event ie = {
        .type = type,
        .code = code,
        .value = value
    };

    write(fd, &ie, sizeof ie);
}

int main() {
    /* These values are very device specific */
    int width = RES_WIDTH;
    int height = RES_HEIGHT;
    int dpi = 96;

    if (width < 1 || height < 1 || dpi < 1)
        fatal("Bad initial value(s).");

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (-1 == fd)
        fatal("open");

    printf("Initializing device screen map as %dx%d\n",
        width, height);

    init(fd, width, height, dpi);

    // /dev/input/eventX writes data in blocks of 24 bytes
    unsigned char buffer[24];
    FILE *ptr;

    ptr = fopen("/dev/input/" TOUCHPAD_EVENT_X, "rb");

    if (ptr == NULL)
      exit(EXIT_FAILURE);

    int raw_x = 0;
    int raw_y = 0;

    while (1) {
        fread(buffer, sizeof(buffer), 1, ptr);

        int code = buffer[CODE_ID_BYTE];

        if (code == ABS_MT_POSITION_X) {
          int len = 0;

          // Size of the value for ABS_MT_POSITION_X is 4 bytes long
          // a string of binary this size with leading zeros in
          // little endian is 33 (that's the reason for loop backwards)
          char binary_string[33];

          for (int i = 23; i >= 20; i--) {
            len += snprintf(binary_string + len,
                sizeof(binary_string) - len, "%8.8b", buffer[i]);
          }

          raw_x = (int) strtol(binary_string, NULL, 2);

        } else if (code == ABS_MT_POSITION_Y) {
          int len = 0;
          char binary_string[33];

          for (int i = 23; i >= 20; i--) {
            len += snprintf(binary_string + len,
                sizeof(binary_string) - len, "%8.8b", buffer[i]);
          }

          raw_y = (int) strtol(binary_string, NULL, 2);
        }

        int pos_x = raw_x * RES_WIDTH / MAX_ABS_X;
        int pos_y = raw_y * RES_HEIGHT / MAX_ABS_Y;

        printf("(%d,%d)\n", pos_x, pos_y);

        /* input is zero-based, but event positions are one-based */
        emit(fd, EV_ABS, ABS_X, 1 + pos_x);
        emit(fd, EV_ABS, ABS_Y, 1 + pos_y);
        emit(fd, EV_SYN, SYN_REPORT, 0);
    }

    puts("Cleaning up...");

    /* give time for events to finish */
    sleep(1);

    if (-1 == ioctl(fd, UI_DEV_DESTROY))
        fatal("ioctl UI_DEV_DESTROY");

    close(fd);
    puts("Goodbye.");
}
