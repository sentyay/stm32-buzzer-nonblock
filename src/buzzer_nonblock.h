#ifndef BUZZER_NONBLOCK_H
#define BUZZER_NONBLOCK_H

#include "buzzer.h"
#include <stddef.h>
#include <stdbool.h>

// Одна нота в последовательности
typedef struct {
    uint32_t pitch;    // частота в Гц, 0 = пауза
    uint32_t duration; // длительность в мс
} BuzzerNote_t;

// Состояние планировщика
typedef struct {
    Buzzer_HandleTypeDef *hbuzzer;
    const BuzzerNote_t   *melody;    // указатель на массив нот
    size_t                size;      // количество нот
    size_t                index;     // текущая нота
    uint32_t              timestamp; // время начала текущей ноты (мс)
    bool                  playing;
} BuzzerScheduler_t;

void BuzzerSched_Init(BuzzerScheduler_t *sched, Buzzer_HandleTypeDef *hbuzzer);
void BuzzerSched_Play(BuzzerScheduler_t *sched, const BuzzerNote_t *melody, size_t size);
void BuzzerSched_Tick(BuzzerScheduler_t *sched); // вызывать каждые 1 мс
bool BuzzerSched_IsPlaying(const BuzzerScheduler_t *sched);

#endif