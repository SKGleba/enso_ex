#ifndef __LV0P_H__
#define __LV0P_H__

#define LV0P_ARG_MAGIC 'LV0P'
#define LV0P_ARGOVERRIDE_ADDR 0x1F850000
#define LV0P_WORKBUF_MIN_SIZE 0x40
struct lv0p_k_s {
    uint32_t id;
    uint32_t off;
    uint32_t sz;
	int ret;
    uint8_t data[0x20];
	union {
    	struct lv0p_k_s *next;
		uint32_t next_pa;
	};
};
struct lv0p_d_s {
	union {
		uint32_t src;
		void *src_va;
	};
	uint32_t dst;
	uint32_t sz;
	union {
		int ret;
		bool ncopyin;
	};
    union {
        struct lv0p_d_s *next;
        uint32_t next_pa;
    };
};
struct lv0p_x_s {
	union {
		int (*func)(uint32_t arg);
		uint32_t addr;
		void *src_va;
	};
	uint32_t arg;
	union {
		uint32_t c_sz;
		int ret;
	};
    union {
        struct lv0p_x_s *next;
        uint32_t next_pa;
    };
};
struct lv0p_arg_s {
	uint32_t magic;
	union {
		uint32_t patcher;
		void *me;
	};
	uint32_t stack;
    union {
        void *w;  // at least 0x40
        uint32_t w_pa;
    };
    union {
        struct lv0p_k_s *k;
        uint32_t k_pa;
    };
	union {
		struct lv0p_d_s *d;
		uint32_t d_pa;
	};
	union {
		struct lv0p_x_s *x;
		uint32_t x_pa;
	};
};

#endif // __LV0P_H__