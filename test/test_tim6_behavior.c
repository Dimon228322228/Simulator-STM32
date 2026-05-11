#include <stdio.h>
#include <string.h>
#include "tim6.h"
#include "nvic.h"

static void tick_timer(TIM6_State *tim6, NVIC_State *nvic, uint32_t ticks) {
    for (uint32_t i = 0; i < ticks; ++i) {
        tim6_update_counter(tim6);
        if (nvic) {
            tim6_update_irq_pending(tim6, nvic);
        }
    }
}

static void clear_uif(TIM6_State *tim6) {
    uint32_t sr_addr = TIM6_BASE_ADDR + TIM6_SR_OFFSET;
    tim6_write_register(tim6, sr_addr, TIM6_SR_UIF); // W1C
}

static int test_basic_period_psc0_arr3(void) {
    printf("[TIM6] test_basic_period_psc0_arr3...\n");
    TIM6_State tim6;
    NVIC_State nvic;
    tim6_init(&tim6);
    nvic_init(&nvic);

    tim6.regs.arr = 3;
    tim6.regs.psc = 0;
    tim6.regs.cr1 |= TIM6_CR1_CEN;
    tim6.regs.dier |= TIM6_DIER_UIE;

    tick_timer(&tim6, &nvic, 3);
    if (tim6.regs.cnt != 3 || (tim6.regs.sr & TIM6_SR_UIF)) {
        printf("  FAIL: CNT/UIF mismatch before overflow\n");
        return 1;
    }

    tick_timer(&tim6, &nvic, 1);
    if (tim6.regs.cnt != 0) {
        printf("  FAIL: CNT not reloaded to 0\n");
        return 1;
    }
    if (!(tim6.regs.sr & TIM6_SR_UIF)) {
        printf("  FAIL: UIF not set on overflow\n");
        return 1;
    }
    if (!nvic_is_pending(&nvic, NVIC_IRQ_TIM6)) {
        printf("  FAIL: NVIC pending not set\n");
        return 1;
    }

    clear_uif(&tim6);
    tim6_update_irq_pending(&tim6, &nvic);
    if (tim6.regs.sr & TIM6_SR_UIF) {
        printf("  FAIL: UIF not cleared\n");
        return 1;
    }
    if (nvic_is_pending(&nvic, NVIC_IRQ_TIM6)) {
        printf("  FAIL: NVIC pending not cleared after UIF clear\n");
        return 1;
    }

    printf("  PASS\n");
    return 0;
}

static int test_psc_scaling_arr3_psc1(void) {
    printf("[TIM6] test_psc_scaling_arr3_psc1...\n");
    TIM6_State tim6;
    tim6_init(&tim6);

    tim6.regs.arr = 3;
    tim6.regs.psc = 1; // CNT increments every 2 ticks
    tim6.regs.cr1 |= TIM6_CR1_CEN;

    tick_timer(&tim6, NULL, 7);
    if (tim6.regs.cnt != 3) {
        printf("  FAIL: CNT expected 3 after 7 ticks, got %u\n", tim6.regs.cnt);
        return 1;
    }
    if (tim6.regs.sr & TIM6_SR_UIF) {
        printf("  FAIL: UIF should not be set yet\n");
        return 1;
    }

    tick_timer(&tim6, NULL, 1);
    if (tim6.regs.cnt != 0 || !(tim6.regs.sr & TIM6_SR_UIF)) {
        printf("  FAIL: Overflow timing incorrect\n");
        return 1;
    }

    printf("  PASS\n");
    return 0;
}

static int test_uif_independent_from_uie(void) {
    printf("[TIM6] test_uif_independent_from_uie...\n");
    TIM6_State tim6;
    NVIC_State nvic;
    tim6_init(&tim6);
    nvic_init(&nvic);

    tim6.regs.arr = 0;
    tim6.regs.psc = 0;
    tim6.regs.cr1 |= TIM6_CR1_CEN;
    tim6.regs.dier &= ~TIM6_DIER_UIE; // disable IRQ

    tick_timer(&tim6, &nvic, 1);

    if (!(tim6.regs.sr & TIM6_SR_UIF)) {
        printf("  FAIL: UIF not set with UIE=0\n");
        return 1;
    }
    if (nvic_is_pending(&nvic, NVIC_IRQ_TIM6)) {
        printf("  FAIL: Pending should not be set when UIE=0\n");
        return 1;
    }

    tim6.regs.dier |= TIM6_DIER_UIE;
    tim6_update_irq_pending(&tim6, &nvic);
    if (!nvic_is_pending(&nvic, NVIC_IRQ_TIM6)) {
        printf("  FAIL: Pending not asserted after enabling UIE with UIF=1\n");
        return 1;
    }

    printf("  PASS\n");
    return 0;
}

static int test_one_shot_delay_ticks(void) {
    printf("[TIM6] test_one_shot_delay_ticks...\n");
    TIM6_State tim6;
    NVIC_State nvic;
    tim6_init(&tim6);
    nvic_init(&nvic);

    tim6.regs.arr = 4; // fire after 5 CNT steps
    tim6.regs.psc = 0;
    tim6.regs.cnt = 0;
    tim6.regs.cr1 |= TIM6_CR1_CEN;
    tim6.regs.dier |= TIM6_DIER_UIE;

    tick_timer(&tim6, &nvic, 4);
    if (tim6.regs.cnt != 4 || (tim6.regs.sr & TIM6_SR_UIF)) {
        printf("  FAIL: Unexpected event before deadline\n");
        return 1;
    }

    tick_timer(&tim6, &nvic, 1);
    if (tim6.regs.cnt != 0 || !(tim6.regs.sr & TIM6_SR_UIF) ||
        !nvic_is_pending(&nvic, NVIC_IRQ_TIM6)) {
        printf("  FAIL: One-shot event missing\n");
        return 1;
    }

    tim6.regs.cr1 &= ~TIM6_CR1_CEN; // simulate ISR stopping timer
    clear_uif(&tim6);
    tim6_update_irq_pending(&tim6, &nvic);
    tick_timer(&tim6, &nvic, 5);
    if (tim6.regs.cnt != 0 || (tim6.regs.sr & TIM6_SR_UIF)) {
        printf("  FAIL: Timer should stay idle when CEN=0\n");
        return 1;
    }

    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_basic_period_psc0_arr3();
    failures += test_psc_scaling_arr3_psc1();
    failures += test_uif_independent_from_uie();
    failures += test_one_shot_delay_ticks();

    if (failures == 0) {
        printf("\nTIM6 behavior tests PASSED.\n");
        return 0;
    }
    printf("\nTIM6 behavior tests FAILED (%d).\n", failures);
    return 1;
}
