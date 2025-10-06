// Uses touchInterruptGetLastStatus which requires ESP32 S2 or S3.

#include <USB.h>
#include <USBHIDKeyboard.h>

// If it was programmed with DEBUGGING true, then you need to hold 0 button while it's booting.
#define DEBUGGING false

#define RGB_BUILTIN 38
#define BRIGHTNESS 50
#define MS_BETWEEN_LOOPS 100
#define PRESS_PER_LOOPS 5
#define PRESS_RELEASE_LOOP 3
#define LOOP_START_NUM 0
#define SLEEP_AFTER_INACTIVE_LOOPS 600

int threshold = 0;  // If 0 is used, benchmark value is used.

bool prev_touch1 = false;
bool prev_touch2 = false;

bool touch1toggle = false;
bool touch2toggle = false;

bool touch1detected = false;
bool touch2detected = false;

int loop_num = LOOP_START_NUM;
int inactive_count = 0;
// int wake_touch_threshold = 30000;

void gotTouch1() {
  // touch1detected = true;
}

void gotTouch2() {
  // touch2detected = true;
}

// Create a keyboard object
USBHIDKeyboard Keyboard;

void setup() {
  Serial.begin(115200);
  if (DEBUGGING) {
    delay(1000);
  }

  // Optional: Set the threshold to 5% of the benchmark value. Only effective if threshold = 0.
  touchSetDefaultThreshold(5);

  touchAttachInterrupt(T9, gotTouch1, threshold);
  touchAttachInterrupt(T3, gotTouch1, threshold);
  touchAttachInterrupt(T8, gotTouch1, threshold);

  touchAttachInterrupt(T11, gotTouch2, threshold);
  touchAttachInterrupt(T12, gotTouch2, threshold);
  touchAttachInterrupt(T13, gotTouch2, threshold);

  if (!DEBUGGING) {
    Keyboard.begin();
    USB.begin();
  }

  touch1toggle = false;
  touch2toggle = false;
  loop_num = LOOP_START_NUM;
  inactive_count = 0;
  // wake_touch_threshold = touchRead(T12) * 1.05;
  Keyboard.release(KEY_DELETE);
  Keyboard.release(KEY_F11);
}

void loop() {
  bool touch1 = touchInterruptGetLastStatus(T9) or touchInterruptGetLastStatus(T3) or touchInterruptGetLastStatus(T8);
  bool touch2 = touchInterruptGetLastStatus(T11) or touchInterruptGetLastStatus(T12) or touchInterruptGetLastStatus(T13);

  if (prev_touch1 == true && touch1 == false) {
    touch1toggle = !touch1toggle;
    if (touch1toggle == true) {
      touch2toggle = false;
    }
    loop_num = LOOP_START_NUM;
    Keyboard.release(KEY_DELETE);
    Keyboard.release(KEY_F11);
  }
  if (prev_touch2 == true && touch2 == false) {
    touch2toggle = !touch2toggle;
    if (touch2toggle == true) {
      touch1toggle = false;
    }
    loop_num = LOOP_START_NUM;
    Keyboard.release(KEY_DELETE);
    Keyboard.release(KEY_F11);
  }
  prev_touch1 = touch1;
  prev_touch2 = touch2;

  if (touch1toggle) {
    if (loop_num == LOOP_START_NUM) {
      neopixelWrite(RGB_BUILTIN, 0, BRIGHTNESS, 0); // Red
      if (DEBUGGING) {
        Serial.println("Pressing DELETE...");
      } else {
        Keyboard.press(KEY_DELETE);
      }
    } else if (loop_num == LOOP_START_NUM + 1) {
      if (DEBUGGING) {
        Serial.println("Releasing DELETE...");
      } else {
        Keyboard.release(KEY_DELETE);
      }
    } else if (loop_num > PRESS_PER_LOOPS / 2) {
      neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    }
  }
  if (touch2toggle) {
    if (loop_num == LOOP_START_NUM) {
      neopixelWrite(RGB_BUILTIN, BRIGHTNESS, 0, 0); // Green
      if (DEBUGGING) {
        Serial.println("Pressing F11...");
      } else {
        Keyboard.press(KEY_F11);
      }
    } else if (loop_num == LOOP_START_NUM + 1) {
      if (DEBUGGING) {
        Serial.println("Releasing F11...");
      } else {
        Keyboard.release(KEY_F11);
      }
    } else if (loop_num > PRESS_PER_LOOPS / 2) {
      neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    }
  }

  if (DEBUGGING && loop_num == LOOP_START_NUM) {
    Serial.printf("touch1 val = %d\n", touchRead(T3));
    Serial.printf("touch2 val = %d\n", touchRead(T12));
    Serial.printf("touch1 last status = %d\n", touchInterruptGetLastStatus(T3));
    Serial.printf("touch2 last status = %d\n", touchInterruptGetLastStatus(T12));
    Serial.printf("inactive_count = %d\n", inactive_count);
    // Serial.printf("threshold = %d\n", wake_touch_threshold);
  }

  if (touch1toggle or touch2toggle){
    loop_num++;
    inactive_count = 0;
    if (loop_num >= PRESS_PER_LOOPS) {
      loop_num = LOOP_START_NUM;
    }
  } else {
    neopixelWrite(RGB_BUILTIN, 0, 0, BRIGHTNESS); // Blue
    inactive_count++;
    if (inactive_count++ > SLEEP_AFTER_INACTIVE_LOOPS) {
      // Most of the time doesn't work and apparently S3 should only work with 1 touch pin.
      // Also tried threshold of 0, seems to work more often but still not great.
      // touchSleepWakeUpEnable(T12, wake_touch_threshold);
      // The following works on all set with touchAttachInterrupt.
      // https://github.com/espressif/arduino-esp32/issues/7150
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
      esp_sleep_enable_touchpad_wakeup();
      neopixelWrite(RGB_BUILTIN, 0, 0, 1);
      for (int i = 0; i < 10; i++) {
        neopixelWrite(RGB_BUILTIN, 0, 0, 0);
        delay(10);
      }
      // if (DEBUGGING) Serial.printf("Starting deep sleep, wake threshold = %d\n", wake_touch_threshold);
      if (DEBUGGING) Serial.println("Starting deep sleep");
      if (DEBUGGING) Serial.flush();
      esp_deep_sleep_start();
    }
  }

  delay(MS_BETWEEN_LOOPS);

}