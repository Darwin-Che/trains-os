#include "hashfunc.h"
#include "lib/include/util.h"
#include "msg/msg.h"
#include "lib/include/uapi.h"
#include "lib/include/assert.h"
#include "lib/include/hashtable.h"

/* Nameserver Hashtable Logic */

#define U_NAME_HT_BUCKSZ 100
#define U_NAME_HT_CAPACITY 1000

#define U_NO_TID_FOUND -2

struct HtNameStr
{
  char str[C_MAX_NS_NAME_SIZE];
};

HT_ELEM_TYPE(struct HtNameStr, int64_t, HtNameElem);

HT_TYPE(HtNameElem, U_NAME_HT_BUCKSZ, U_NAME_HT_CAPACITY, HtName);

static uint64_t hash_func_name(const struct HtNameStr *htname);
static bool hash_func_cmp(const struct HtNameStr *htname1, const struct HtNameStr *htname2);

#define HT_HASH_FUNC(ELEM) hash_func_name

#define HT_CMP_FUNC(ELEM) hash_func_cmp

/* Namserver Logic */

void ns_register_name_with_tid(struct HtName *ns_hashtable, const char *name, int64_t tid)
{
  struct HtNameElem *elem;
  HT_FIND(ns_hashtable, (const struct HtNameStr *)name, elem);

  if (elem == NULL)
  {
    HT_GET_FREE(ns_hashtable, elem);
    util_strncpy(elem->key.str, C_MAX_NS_NAME_SIZE, name);
    elem->val = tid;
    HT_INSERT(ns_hashtable, elem);
  }
  else
  {
    // overwrite element's tid
    elem->val = tid;
  }
}

int64_t ns_whois_tid(struct HtName *ns_hashtable, const char *name)
{
  struct HtNameElem *elem;

  HT_FIND(ns_hashtable, (const struct HtNameStr *)name, elem);
  if (elem == NULL)
  {
    return U_NO_TID_FOUND;
  }
  return elem->val;
}

void ns_main()
{
  // printf("NameServer is starting up!\r\n");
  int my_tid = ke_my_tid();
  ASSERT_MSG(my_tid == ke_nameserver_tid, "Nameserver should have tid %d (predefined), instead of %d\r\n", ke_nameserver_tid, my_tid);

  struct HtName ns_hashtable;
  HT_INIT(&ns_hashtable, U_NAME_HT_BUCKSZ, U_NAME_HT_CAPACITY);

  int tid;
  MSGBOX_T(reqbox, 11);

  while (1)
  {
    MSG_RECV(&tid, &reqbox);
    if (MSGBOX_IS(&reqbox, mNsRegisterReq))
    {
      MSGBOX_CAST(&reqbox, mNsRegisterReq, req);
      ns_register_name_with_tid(&ns_hashtable, req->name, tid);
      MSG_INIT(reply, mEmptyMsg);
      MSG_REPLY(tid, &reply);
    }
    else if (MSGBOX_IS(&reqbox, mNsWhoisReq))
    {
      MSGBOX_CAST(&reqbox, mNsWhoisReq, req);
      int found_tid = ns_whois_tid(&ns_hashtable, req->name);
      MSG_INIT(reply, mNsWhoisResp);
      reply.tid = found_tid;
      MSG_REPLY(tid, &reply);
    }
    else
    {
      printf("NameServer: Invalid Request received\r\n");
      assert_fail();
    }
  }
}

/* Hashtable Helpers */

// I want to use the string hash in
// https://cp-algorithms.com/string/string-hashing.html#calculation-of-the-hash-of-a-string

// Or check that is hash function works well against collision
static uint64_t hash_func_name(const struct HtNameStr *htname)
{
  return hashfunc_string(htname->str);
}

static bool hash_func_cmp(const struct HtNameStr *htname1, const struct HtNameStr *htname2)
{
  return util_strcmp(htname1->str, htname2->str);
}