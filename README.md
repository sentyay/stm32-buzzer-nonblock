# stm32-buzzer (non-blocking fork)

Fork of [brapacz/stm32-buzzer](https://github.com/brapacz/stm32-buzzer) with added **non-blocking melody playback** via a SysTick-driven scheduler.

Original library plays notes using `HAL_Delay`, which blocks the main loop. This fork adds `buzzer_nonblock.c/.h` — a lightweight scheduler that advances the melody in the SysTick interrupt, leaving the main loop completely free for display updates, button handling, and other tasks.

---

## Files

| File | Description |
|---|---|
| `buzzer.c` / `buzzer.h` | Original library — PWM tone generation via HAL timer |
| `buzzer_tones.h` | Note frequency definitions (B0 – D#8) |
| `buzzer_nonblock.c` / `buzzer_nonblock.h` | Non-blocking scheduler (this fork) |

---

## Hardware setup

Configure a timer with PWM output in STM32CubeMX. Example for **TIM4 Channel 3**:

```
Counter:
    Prescaler:            8-1
    Counter Mode:         UP
    Counter Period (ARR): 0xFFFF
    Clock Division:       No Division
    Auto-reload preload:  Disable

PWM Generation Channel 3:
    Mode:        PWM Mode 1
    Pulse:       50-1
    Fast Mode:   Disable
    CH Polarity: High

GPIO:
    User label:           BUZZER
    Maximum output speed: HIGH
```

---

```

In STM32CubeIDE / System Workbench, add `ExternalLibs/Buzzer/src` to:
- **Project → Properties → C/C++ General → Paths and Symbols → Includes**
- **Project → Properties → C/C++ General → Paths and Symbols → Source Locations**

---

## Integration

### `main.c`

```c
/* Private includes */
#include "buzzer.h"
#include "buzzer_nonblock.h"
#include "buzzer_tones.h"

/* USER CODE BEGIN PV */
Buzzer_HandleTypeDef hbuzzer;
BuzzerScheduler_t    hbuzzerSched;

// Button click sound
static const BuzzerNote_t snd_click[] = {
    { NOTE_A5, 30 },
    { 0,       20 },
    { NOTE_E6, 20 },
};

// Startup melody
static const BuzzerNote_t snd_startup[] = {
    { NOTE_C5, 100 },
    { NOTE_E5, 100 },
    { NOTE_G5, 150 },
    { 0,        50 },
};
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
Buzzer_InitTypeDef buzzerConfig;
buzzerConfig.channel          = TIM_CHANNEL_3;
buzzerConfig.timer            = &htim4;
buzzerConfig.timerClockFreqHz = HAL_RCC_GetPCLK2Freq();
Buzzer_Init(&hbuzzer, &buzzerConfig);
Buzzer_Start(&hbuzzer);

BuzzerSched_Init(&hbuzzerSched, &hbuzzer);
BuzzerSched_Play(&hbuzzerSched, snd_startup,
                 sizeof(snd_startup) / sizeof(snd_startup[0]));
/* USER CODE END 0 */

/* USER CODE BEGIN WHILE */
while (1) {
    if (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) {
        HAL_Delay(20); // debounce
        if (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) {
            BuzzerSched_Play(&hbuzzerSched, snd_click,
                             sizeof(snd_click) / sizeof(snd_click[0]));
        }
        while (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET);
    }

    Display_Update(); // not blocked by sound
}
```

### `stm32f1xx_it.c`

```c
/* USER CODE BEGIN Includes */
#include "buzzer_nonblock.h"
/* USER CODE END Includes */

/* USER CODE BEGIN EV */
extern BuzzerScheduler_t hbuzzerSched;
/* USER CODE END EV */

void SysTick_Handler(void)
{
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  HAL_SYSTICK_IRQHandler(); // required to trigger HAL_SYSTICK_Callback
  /* USER CODE END SysTick_IRQn 1 */
}

/* USER CODE BEGIN 4 */
void HAL_SYSTICK_Callback(void) {
    BuzzerSched_Tick(&hbuzzerSched);
}
/* USER CODE END 4 */
```

> **Note:** `HAL_SYSTICK_IRQHandler()` must be present in `SysTick_Handler` — CubeMX does not add it by default. Without it, `HAL_SYSTICK_Callback` is never called and the scheduler does not advance.

---

## API

```c
// Initialize the scheduler (call once after Buzzer_Init)
void BuzzerSched_Init(BuzzerScheduler_t *sched, Buzzer_HandleTypeDef *hbuzzer);

// Start playing a melody (interrupts any currently playing sound)
void BuzzerSched_Play(BuzzerScheduler_t *sched, const BuzzerNote_t *melody, size_t size);

// Advance the scheduler — call every 1 ms (from HAL_SYSTICK_Callback)
void BuzzerSched_Tick(BuzzerScheduler_t *sched);

// Returns true while a melody is playing
bool BuzzerSched_IsPlaying(const BuzzerScheduler_t *sched);
```

### `BuzzerNote_t`

```c
typedef struct {
    uint32_t pitch;    // frequency in Hz, 0 = silence/rest
    uint32_t duration; // duration in ms
} BuzzerNote_t;
```

---

## How it works

```
SysTick IRQ (every 1 ms)
    └── HAL_SYSTICK_IRQHandler()
            └── HAL_SYSTICK_Callback()
                    └── BuzzerSched_Tick()
                            ├── elapsed < duration → do nothing (~5 ns)
                            └── elapsed ≥ duration → Buzzer_Note(next pitch)
```

`Buzzer_Note()` only writes to the timer's ARR/PSC registers — safe to call from an interrupt.

---

## Wiring

```
                              ___ 3V3
                               |
                               |
                               |    __
                               |___|  \
                                   |   |  Buzzer / Speaker / Piezo
                                ___|   |
                               |   |__/
                               |
                            ___|___
          330 Ω            /   /   \
         __________       /  |/     \
MCU _____|          |_____|__|       |  BC817 NPN
         |__________|     |  |\     |
                          \  | \    /
                           \___|___/
                               |
                              GND
```

---

## Tested on

- STM32F103C8T6 (Blue Pill)

---

## Credits

Original library by [brapacz](https://github.com/brapacz/stm32-buzzer).  
Non-blocking scheduler added in this fork.
