// This method rapidly moves the mouse towards the top left corner
// in an extremely negative direction to guarentee that it is at
// (0,0). Then the touchpad MUST be disabled (there should be a
// button on the keyboard to do this) in order to guarantee that
// the position is known at all times. Otherwise, we will have to
// move it to (0,0) each loop in order to guarantee position.
//
// This seems to work EXTREMELY well unless we need to use
// mouse buttons.

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/uinput.h>
#include <sys/ioctl.h>

// The below values are device specific and should be changed by the user

#define RES_WIDTH 1920
#define RES_HEIGHT 1200

// This is where the user needs to run `evtest` as root
// and move their finger to the bottom right corner
// of their touchpad to determine the maximum values
// Ie: `sudo evtest /dev/input/eventX |& grep ABS`
// where "eventX" is replaced by the event number of
// your touchpad. Eg: "event5"
#define MAX_ABS_X 3528
#define MAX_ABS_Y 1793

// The final values to configure is the measurements
// for you touchpad dimensions. Use a ruler or tape
// to find the length and width IN MILLIMETERS
#define TOUCHPAD_WIDTH_MM 114
#define TOUCHPAD_HEIGHT_MM 58

// The final final values to configure are the
// measurements for the area you want to use/work in
// eg: I want to use 75mmx55mm like on tablet so
#define ADJUSTED_WIDTH_MM 114
#define ADJUSTED_HEIGHT_MM 58

static void emit(int fd, int type, int code, int value) {
  struct input_event ie = {.type = type, .code = code, .value = value};

  write(fd, &ie, sizeof(ie));
}

int main(void) {
  // Get event* number for trackpad
  FILE *cmd;
  char result[1024];

  cmd = popen("cat /proc/bus/input/devices |"
              "grep -i touchpad -A 4 | grep -oP '(?<=event)[0-9]*'",
              "r");
  if (cmd == NULL) {
    perror("popen");
    exit(EXIT_FAILURE);
  }

  fseek(cmd, 0, SEEK_END);
  // below length doesn't work so hardcoding it
  // int length = ftell(cmd);
  int length = 3;
  fseek(cmd, 0, SEEK_SET);

  char *touchpad_event_x = malloc(sizeof(*touchpad_event_x) * (length + 1));

  char c;
  int i = 0;
  while (i < length && (c = fgetc(cmd)) != EOF && c != '\n') {
    touchpad_event_x[i] = c;
    i++;
  }
  touchpad_event_x[i] = '\0';

  pclose(cmd);

  // Setup emulated device struct
  // Why is clang-format freaking out??
  struct uinput_setup device = {.id =
                                    {
                                        .bustype = BUS_USB,
                                    },
                                .name =
                                    "Emulated \"Absolute\" Positioning Device"};

  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

  if (fd == -1) {
    perror("Cannot read from file. Most likely need to run as root.\n");
    exit(EXIT_FAILURE);
  }

  // enable mouse button left and relative events
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);

  ioctl(fd, UI_SET_EVBIT, EV_REL);
  ioctl(fd, UI_SET_RELBIT, REL_X);
  ioctl(fd, UI_SET_RELBIT, REL_Y);

  // device creation
  ioctl(fd, UI_DEV_SETUP, &device);
  ioctl(fd, UI_DEV_CREATE);

  /* give time for device creation */
  sleep(1);

  // /dev/input/eventX writes data in blocks of 24 bytes
  unsigned char buffer[24];
  FILE *ptr;

  char filename[50] = "/dev/input/event";
  strncat(filename, touchpad_event_x, strnlen(touchpad_event_x, 2));

  // Open eventX file
  ptr = fopen(filename, "rb");

  if (ptr == NULL) {
    perror("Couldn't open file. Need to be in \"input\""
           "user group most likely.\n");
    exit(EXIT_FAILURE);
  }

  int touchpad_fd = open(filename, O_RDONLY | O_NONBLOCK);
  // Disable touchpad to only use this program aka GRAB IT
  ioctl(touchpad_fd, EVIOCGRAB, 1);

  // system("gsettings set org.gnome.desktop.peripherals.touchpad "
  //    "send-events 'disabled'");

  int raw_x = 0;
  int raw_y = 0;

  // Speed improvemnts by not settings position if already there
  int prev_raw_x = 0;
  int prev_raw_y = 0;

  int cur_pos_x = 0;
  int cur_pos_y = 0;

  // GRAB

  // Move to 0,0 in order to reset
  emit(fd, EV_REL, REL_X, INT_MIN);
  emit(fd, EV_REL, REL_Y, INT_MIN);
  emit(fd, EV_SYN, SYN_REPORT, 0);
  usleep(500);

  // Code works up to this point

  while (1) {
    fread(buffer, sizeof(buffer), 1, ptr);

    // Add code bytes to calculate code
    int code_len = 0;
    char code_bin[17]; // Why is this 17 and not 16?

    for (int i = 19; i >= 18; i--) {
      code_len += snprintf(code_bin + code_len, sizeof(code_bin) - code_len,
                           "%8.8b", buffer[i]);
    }

    int code = (int)strtol(code_bin, NULL, 2);

    if (code == ABS_MT_POSITION_X) {
      int len = 0;

      // Size of the value for ABS_MT_POSITION_X is 4 bytes long
      // a string of binary this size with leading zeros in
      // little endian is 33 (that's the reason for loop backwards)
      char binary_string[33];

      for (int i = 23; i >= 20; i--) {
        len += snprintf(binary_string + len, sizeof(binary_string) - len,
                        "%8.8b", buffer[i]);
      }

      raw_x = (int)strtol(binary_string, NULL, 2);

      // Skip calculations and setting position
      // Saves time and resources
      if (raw_x == prev_raw_x)
        continue;

    } else if (code == ABS_MT_POSITION_Y) {
      int len = 0;
      char binary_string[33];

      for (int i = 23; i >= 20; i--) {
        len += snprintf(binary_string + len, sizeof(binary_string) - len,
                        "%8.8b", buffer[i]);
      }

      raw_y = (int)strtol(binary_string, NULL, 2);

      // Then skip this 24 bytes
      if (raw_y == prev_raw_y)
        continue;

    } else if (code == BTN_LEFT) {
      int len = 0;
      char binary_string[33];

      for (int i = 23; i >= 20; i--) {
        len += snprintf(binary_string + len, sizeof(binary_string) - len,
                        "%8.8b", buffer[i]);

        int raw_click = (int)strtol(binary_string, NULL, 2);

        // If raw_click is 0 it will be depress if its 1
        // it will be press down
        emit(fd, EV_KEY, BTN_LEFT, raw_click);
      }
    } else {
      // No reason to do more calculations.
      continue;
    }

    double scale_factor_x = ((double)MAX_ABS_X) / TOUCHPAD_WIDTH_MM;
    double scale_factor_y = ((double)MAX_ABS_Y) / TOUCHPAD_HEIGHT_MM;

    int scaled_width = (int)(scale_factor_x * ADJUSTED_WIDTH_MM);
    int scaled_height = (int)(scale_factor_y * ADJUSTED_HEIGHT_MM);

    int uncentered_pos_x = raw_x * RES_WIDTH / scaled_width;
    int uncentered_pos_y = raw_y * RES_HEIGHT / scaled_height;

    int leftover_width = MAX_ABS_X - scaled_width;
    int leftover_height = MAX_ABS_Y - scaled_height;

    // this works to center area
    int pos_x = uncentered_pos_x - leftover_width / 2;
    int pos_y = uncentered_pos_y - leftover_height / 2;

    printf("(%d,%d)\n", pos_x, pos_y);

    int rel_pos_x = pos_x - cur_pos_x;
    int rel_pos_y = pos_y - cur_pos_y;

    emit(fd, EV_REL, REL_X, rel_pos_x);
    emit(fd, EV_REL, REL_Y, rel_pos_y);
    emit(fd, EV_SYN, SYN_REPORT, 0);

    // Disallow going over boundaries of the screen
    // for current position calculation so we can get
    // true pos.
    if (pos_x < 0)
      pos_x = 0;
    if (pos_x > RES_WIDTH)
      pos_x = RES_WIDTH;
    if (pos_y < 0)
      pos_y = 0;
    if (pos_y > RES_HEIGHT)
      pos_y = RES_HEIGHT;

    // After movement this is true
    cur_pos_x = pos_x;
    cur_pos_y = pos_y;

    prev_raw_x = raw_x;
    prev_raw_y = raw_y;
  }

  puts("Cleaning up...");

  free(touchpad_event_x);

  /* give time for events to finish */
  sleep(1);

  ioctl(touchpad_fd, EVIOCREVOKE, 1);
  close(touchpad_fd);

  ioctl(fd, UI_DEV_DESTROY);

  // Re-enable usual touchpad driver
  // system("gsettings set org.gnome.desktop.peripherals.touchpad "
  //    "send-events 'enabled'");

  close(fd);
  puts("Goodbye.");
}
