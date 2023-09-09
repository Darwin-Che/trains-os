#ifndef K_SYS_VAL_H
#define K_SYS_VAL_H

struct PgMgr;

struct SysAddr
{
  struct PgMgr *pgmgr;
  struct SlabMgr *slabmgr;
};

extern struct SysAddr SYSADDR;

#endif