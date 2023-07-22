#ifndef K_SYS_VAL_H
#define K_SYS_VAL_H

struct PgMgr;

struct SysAddr {
  struct PgMgr *pgmgr;
};

extern struct SysAddr SYSADDR;

#endif