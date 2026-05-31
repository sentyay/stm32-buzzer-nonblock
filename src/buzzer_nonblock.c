#include "buzzer_nonblock.h"

void BuzzerSched_Init(BuzzerScheduler_t *sched, Buzzer_HandleTypeDef *hbuzzer) {
    sched->hbuzzer   = hbuzzer;
    sched->melody    = NULL;
    sched->size      = 0;
    sched->index     = 0;
    sched->timestamp = 0;
    sched->playing   = false;
}

void BuzzerSched_Play(BuzzerScheduler_t *sched, const BuzzerNote_t *melody, size_t size) {
    sched->melody    = melody;
    sched->size      = size;
    sched->index     = 0;
    sched->timestamp = HAL_GetTick();
    sched->playing   = true;
    // Сразу играем первую ноту
    Buzzer_Note(sched->hbuzzer, melody[0].pitch);
}

void BuzzerSched_Tick(BuzzerScheduler_t *sched) {
    if (!sched->playing) return;

    uint32_t elapsed = HAL_GetTick() - sched->timestamp;

    if (elapsed >= sched->melody[sched->index].duration) {
        sched->index++;
        if (sched->index >= sched->size) {
            // Мелодия закончилась
            Buzzer_Note(sched->hbuzzer, 0);
            sched->playing = false;
            return;
        }
        sched->timestamp = HAL_GetTick();
        Buzzer_Note(sched->hbuzzer, sched->melody[sched->index].pitch);
    }
}

bool BuzzerSched_IsPlaying(const BuzzerScheduler_t *sched) {
    return sched->playing;
}