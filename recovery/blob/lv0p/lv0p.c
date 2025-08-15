#include "hardware/types.h"
#include "hardware/paddr.h"
#include "hardware/maika.h"

typedef int bool;
#include "lv0p.h"

#define NULL ((void *)0)

#define AME_COUNT 3
struct actually_me_s {
	void *start;
	void *(*memcpy)(void *dest, const void *src, uint32_t n);
    int (*patch_vis_keyslot)(struct actually_me_s *me, uint32_t id, uint32_t off, uint32_t sz, void *data);
};
struct actually_me_s actually_me;

void *memcpy(void *dest, const void *src, uint32_t n) {
    if (((uint32_t)src | (uint32_t)dest | (uint32_t)n) & 3) {
        const uint8_t *s = src;
        uint8_t *d = dest;
        while (n) {
            *d++ = *s++;
            n--;
        }
    } else {
        const uint32_t *s = src;
        uint32_t *d = dest;
        while (n) {
            *d++ = *s++;
            n -= 4;
        }
    }
    return dest;
}

int patch_vis_keyslot(struct actually_me_s *me, uint32_t id, uint32_t off, uint32_t sz, void *data) {
    uint8_t buf[MAIKA_KEYSLOT_SIZE];
    maika_s *maika = (maika_s *)MAIKA_OFFSET;
    me->memcpy(buf, (const void *)maika->keyring[id - MAIKA_KEYSLOT_COUNT], MAIKA_KEYSLOT_SIZE);
	if (off + sz > MAIKA_KEYSLOT_SIZE) return -1;
    me->memcpy(buf + off, (const void *)data, sz);
    me->memcpy((void *)maika->keyring_ctrl.data, (const void *)buf, MAIKA_KEYSLOT_SIZE);
    maika->keyring_ctrl.keyslot = id;
	return 1;
}

__attribute__((section(".text.start"))) int start(struct lv0p_arg_s *arg) {
	if (*(uint32_t *)LV0P_ARGOVERRIDE_ADDR == LV0P_ARG_MAGIC)
		arg = (struct lv0p_arg_s *)LV0P_ARGOVERRIDE_ADDR;
    if (arg->magic != LV0P_ARG_MAGIC) return -1;
    struct actually_me_s *me = (struct actually_me_s *)(((uint32_t)arg->me + (uint32_t)&actually_me));
	uint32_t *funcs = (uint32_t *)(arg->me + (uint32_t)&actually_me);
	for (int i = 0; i < AME_COUNT; i++)
		funcs[i] += (uint32_t)arg->me;  // adjust pointers to be relative to the payload base
	struct lv0p_k_s *k = arg->k;
	while (k) {
        struct lv0p_k_s *ck = ((struct lv0p_k_s *)arg->w);
        me->memcpy(ck, k, sizeof(struct lv0p_k_s));
        ck->ret = me->patch_vis_keyslot(me, ck->id, ck->off, ck->sz, ck->data);
        me->memcpy(k, ck, sizeof(struct lv0p_k_s));
        k = ck->next;
	}
	struct lv0p_d_s *d = arg->d;
	while (d) {
        struct lv0p_d_s *cd = ((struct lv0p_d_s *)arg->w);
        me->memcpy(cd, d, sizeof(struct lv0p_d_s));
		cd->ret = (int)me->memcpy((void *)cd->dst, (const void *)cd->src, cd->sz);
        me->memcpy(d, cd, sizeof(struct lv0p_d_s));
        d = cd->next;
	}
	struct lv0p_x_s *x = arg->x;
	while (x) {
        struct lv0p_x_s *cx = ((struct lv0p_x_s *)arg->w);
        me->memcpy(cx, x, sizeof(struct lv0p_x_s));
		cx->ret = cx->func(cx->arg);
        me->memcpy(x, cx, sizeof(struct lv0p_x_s));
        x = cx->next;
	}
	return 0;
}

struct actually_me_s actually_me = {
	.start = start,
	.memcpy = memcpy,
	.patch_vis_keyslot = patch_vis_keyslot
};