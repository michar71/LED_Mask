

typedef enum{
  MODE_DISTANCE_WARNING,
  MODE_DISTANCE_RAINBOW,
  MODE_DISTANCE_BINARY,
  MODE_PULSE_COLOR,
  MODE_PULSE_RED,
  MODE_BEAT,
  MODE_LAST
}t_mode;


typedef struct{
    int mode;
}setup_t;


#define LONG_PRESS_MS 1000
#define VERY_LONG_PRESS_MS 5000
#define VERY_VERY_LONG_PRESS_MS 10000
typedef enum{
    NO_PRESS,
    DOWN,
    SHORT_PRESS,
    LONG_PRESS,
    VERY_LONG_PRESS,
    VERY_VERY_LONG_PRESS
}mode_button_e;

void set_rgb_led(uint8_t R, uint8_t G, uint8_t B);