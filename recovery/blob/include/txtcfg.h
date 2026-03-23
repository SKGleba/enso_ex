#ifndef __TXTCFG_H__
#define __TXTCFG_H__

#include "utils.h"

enum TXTCFG_ARG_TYPES {
    TXTCFG_ARG_TYPE_ASCII = 0,
    TXTCFG_ARG_TYPE_UINT,
    TXTCFG_ARG_TYPE_RDATA,
    TXTCFG_ARG_TYPE_FDATA,
};

#define TXTCFG_GET_BITPOS(_idx, _type) (((_idx) * 4) + (TXTCFG_ARG_TYPE##_type))
#define _TXTCFG_TYPES_ALLOW1(_idx, _type1) (BITN(TXTCFG_GET_BITPOS(_idx, _type1)))
#define _TXTCFG_TYPES_ALLOW2(_idx, _type1, _type2) (_TXTCFG_TYPES_ALLOW1(_idx, _type1) | BITN(TXTCFG_GET_BITPOS(_idx, _type2)))
#define _TXTCFG_TYPES_ALLOW3(_idx, _type1, _type2, _type3) (_TXTCFG_TYPES_ALLOW2(_idx, _type1, _type2) | BITN(TXTCFG_GET_BITPOS(_idx, _type3)))
#define TXTCFG_TYPES_ALLOW(...) FUN_VAR4(__VA_ARGS__, _TXTCFG_TYPES_ALLOW3, _TXTCFG_TYPES_ALLOW2, _TXTCFG_TYPES_ALLOW1)(__VA_ARGS__)
#define TXTCFG_TYPES_ALLOW_ALL(_idx) (TXTCFG_TYPES_ALLOW((_idx), _UINT, _RDATA, _FDATA) | BITN(TXTCFG_GET_BITPOS((_idx), _ASCII)))
#define TXTCFG_TYPES_PARSE(...) BITNVAL(16, TXTCFG_TYPES_ALLOW(__VA_ARGS__))
#define TXTCFG_TYPES_PARSE_ALL(_idx) BITNVAL(16, TXTCFG_TYPES_ALLOW_ALL(_idx))

struct txtcfg_arg_s {
	const char *name;
    struct {
        const int min_len;
        const int max_len;  // we assume no args larger than signed int +range...
        int act_len;
        union {
            uint32_t uintgr;
            void *data;
            char *ascii;
        };
    } uarg[4];
    uint32_t types;
    int (*handler)();
    bool exec;  // execute handler immediately upon finding the occurrence
    bool parsed;
};

struct txtcfg_s {
	char *fpath;
    struct {
        void *va;
        uint32_t size;
    } buf;
    const int arg_count;
	const int arg_hardc;
    struct txtcfg_arg_s *arg;
	const void *(*h_dispatcher)(int idx);
	int (*brhandler)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
};

enum CMDH_ENUMS {
    CMDH_INVALID = 0,
    CMDH_LIVEQUE,
    CMDH_ERRBREAK,
    CMDH_USTART,
    CMDH_MNTINIT = CMDH_USTART,
    CMDH_LV0INIT,
    CMDH_LV0_KSP,
    CMDH_LV0_DAT,
    CMDH_LV0_EXE,
    CMDH_ARM_DAT,
    CMDH_ARM_EXE,
    CMDH_KBLPARM,
    CMDH_DCOUNT
};

#ifndef RXP_PIE
void txtcfg_parse(struct txtcfg_s *cfg);
void txtcfg_cleanup(struct txtcfg_s *cfg);
int txtcfg_loadExec(struct txtcfg_s *cfg, bool cleanup);
int txtcfg_lxPath(char *path, bool cleanup, struct txtcfg_s *cfg);

extern struct txtcfg_arg_s cmdh_args[CMDH_DCOUNT];
extern struct lv0p_arg_s cmdh_lv0p_args;
extern struct armp_arg_s cmdh_armp_args;
const void *cmdh_dispatch_table(int idx);
int cmdh_allow_livexe(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_breakproxy(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_ks(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_lv0dat(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_lv0x(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_kblp(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_armd(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_armx(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_mntinit(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_lv0init(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
int cmdh_apply_pchains(struct lv0p_arg_s *lv0c, struct armp_arg_s *armc, bool cleanup);
int cmdh_lxp(char *fpath);
#else
#define r_txtcfg_parse(...) _r->txtcfg->txtcfg_parse(__VA_ARGS__)
#define r_txtcfg_cleanup(...) _r->txtcfg->txtcfg_cleanup(__VA_ARGS__)
#define r_txtcfg_loadExec(...) _r->txtcfg->txtcfg_loadExec(__VA_ARGS__)
#define r_txtcfg_lxPath(...) _r->txtcfg->txtcfg_lxPath(__VA_ARGS__)
#define r_cmdh_args(...) _r->txtcfg->cmdh_args(__VA_ARGS__)
#define r_cmdh_lv0p_args(...) _r->txtcfg->cmdh_lv0p_args(__VA_ARGS__)
#define r_cmdh_armp_args(...) _r->txtcfg->cmdh_armp_args(__VA_ARGS__)
#define r_cmdh_dispatch_table(...) _r->txtcfg->cmdh_dispatch_table(__VA_ARGS__)
#define r_cmdh_allow_livexe(...) _r->txtcfg->cmdh_allow_livexe(__VA_ARGS__)
#define r_cmdh_breakproxy(...) _r->txtcfg->cmdh_breakproxy(__VA_ARGS__)
#define r_cmdh_ks(...) _r->txtcfg->cmdh_ks(__VA_ARGS__)
#define r_cmdh_lv0dat(...) _r->txtcfg->cmdh_lv0dat(__VA_ARGS__)
#define r_cmdh_lv0x(...) _r->txtcfg->cmdh_lv0x(__VA_ARGS__)
#define r_cmdh_kblp(...) _r->txtcfg->cmdh_kblp(__VA_ARGS__)
#define r_cmdh_armd(...) _r->txtcfg->cmdh_armd(__VA_ARGS__)
#define r_cmdh_armx(...) _r->txtcfg->cmdh_armx(__VA_ARGS__)
#define r_cmdh_mntinit(...) _r->txtcfg->cmdh_mntinit(__VA_ARGS__)
#define r_cmdh_lv0init(...) _r->txtcfg->cmdh_lv0init(__VA_ARGS__)
#define r_cmdh_apply_pchains(...) _r->txtcfg->cmdh_apply_pchains(__VA_ARGS__)
#define r_cmdh_lxp(...) _r->txtcfg->cmdh_lxp(__VA_ARGS__)
#endif

struct exports_txtcfg_s {
    void (*txtcfg_parse)(struct txtcfg_s *cfg);
    void (*txtcfg_cleanup)(struct txtcfg_s *cfg);
    int (*txtcfg_loadExec)(struct txtcfg_s *cfg, bool cleanup);
    int (*txtcfg_lxPath)(char *path, bool cleanup, struct txtcfg_s *cfg);
    struct txtcfg_arg_s *cmdh_args;
    struct lv0p_arg_s *cmdh_lv0p_args;
    struct armp_arg_s *cmdh_armp_args;
	const void *(*cmdh_dispatch_table)(int idx);
	int (*cmdh_allow_livexe)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_breakproxy)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_ks)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_lv0dat)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_lv0x)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_kblp)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_armd)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_armx)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_mntinit)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_lv0init)(int idx, struct txtcfg_arg_s *arg, struct txtcfg_s *cfg);
	int (*cmdh_apply_pchains)(struct lv0p_arg_s *lv0c, struct armp_arg_s *armc, bool cleanup);
	int (*cmdh_lxp)(char *fpath);
};

#endif // __TXTCFG_H__