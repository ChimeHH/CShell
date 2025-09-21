#include "zType_Def.h"
#include "zSalOS.h"
#include "cshell.h"
#include "zTraceApi.h"


#include "zFootprint.h"
#include "zFootprintApi.h"


int g_zEosEnabled=1;

/* simple eosNHash of fixed size */
#define EOS_NHASH 19997
Eos_t eosNHash[EOS_NHASH] = { {0, }, };

/* errstr table */
/* hash a errstr */
static unsigned ErrnoValue(const char *errstr)
{
  return *(unsigned*)errstr;
}

int zEosPeg(const char* errstr, dword_t param0, dword_t param1, const char* function, int line)
{
  if(!g_zEosEnabled || !errstr) return -1;
 
  int num = ErrnoValue(errstr) % EOS_NHASH;
  int count = EOS_NHASH;
  
  while(--count >= 0)
  {
    Eos_t *ptr = &eosNHash[num];

    if(!ptr->errstr) //not exist yet
    {
      ptr->errstr = errstr;
      ptr->param0 = param0; //replace the last params
      ptr->param1 = param1;
      
      ptr->function = function;
      ptr->line = line;
      ptr->count = 1;
      ptr->when = zTime();
      
      return 1;
    }
    else if(ptr->errstr == errstr && ptr->function == function && ptr->line == line)
    {
      ptr->count += 1;
      ptr->when = zTime();

      return 2;
    }
    
    if(++num >= EOS_NHASH) num=0; /* try the next entry */
  }

  //overflow
  return -1;
}



int zEosShow(const char* errstr)
{
  int num;

  zTraceP("EOS Enabled: %s\n", g_zEosEnabled?"YES":"NO");

  for(num=0; num<EOS_NHASH; num++)
  {
    Eos_t *ptr = &eosNHash[num];

    if(!ptr->errstr) continue;
    if(errstr)
    {
      if(!strcasestr(ptr->errstr, errstr)) continue;

      zTraceP("[%5d]: %s  %d [%08x %08x]  -- %s:%d  %s\n", num, ptr->errstr, ptr->count, ptr->param0, ptr->param1,
                  ptr->function, ptr->line, zCTime(&ptr->when));
    }
    else
    {
      zTraceP("[%5d]: %s  %d [%08x %08x]  -- %s:%d  %s\n", num, ptr->errstr, ptr->count, ptr->param0, ptr->param1,  
                  ptr->function, ptr->line, zCTime(&ptr->when));
    }
  }

  return 0;
}

int zEosReset()
{
  memset(eosNHash, 0, sizeof(eosNHash));
  return 0;
}

void* zFootprintInit(int maxFootprints, int bReentrant)
{
  //actual table size is 1+maxFootprints
  int len = sizeof(Footprint_t) * maxFootprints + sizeof(FpService_t);
  
  FpService_t *tab = (FpService_t *)malloc(len);
  
  memset(tab, 0, len);

  tab->maxCount = maxFootprints;
  if(bReentrant) tab->lock = zMutexInit();

  return tab;
}


int zFootprintShow(void* serviceCore, int count)
{
  FpService_t *tab = (FpService_t *)serviceCore;  
  if(!tab) return -1;
  
  count = MIN(count, tab->maxCount);
  int begin = tab->next - count;
  
  zTraceP("Configured Max Count: %d, Next: %d\n", tab->maxCount, tab->next);
  zTraceP("Print Required Count: %d\n", count);

  int i;
  for(i=0; i<count; i++)
  {
    int pos = (begin + i + tab->maxCount) % tab->maxCount;
    Footprint_t *fp = &tab->fp[pos];
    if(!fp->errstr) continue;

    zTraceP("%4d %4d - %64s %4d %s\n", i, pos, symFuncNameEval(fp->func), fp->line, fp->errstr);
  }

  return 0;
}


int zFootprintAdd(void* serviceCore, const char* errstr, const char* function, int line)
{
  FpService_t *tab = (FpService_t *)serviceCore;  
  if(!tab) return -1;

  if(tab->lock) zMutexLock(tab->lock);
  
  Footprint_t *fp = &tab->fp[++tab->next % tab->maxCount];

  fp->func = (*(dword_t*)&function) & 0xFFFFF;
  fp->errstr = errstr;
  fp->line = line & 0xFFF;
  
  if(tab->lock) zMutexUnlock(tab->lock);

  return 0;
}

#define FOOTPRINT_SELFTEST 1
#if FOOTPRINT_SELFTEST
int testFootprint() //test compile warnings...
{
  zFootprintAdd(0, "test", __FUNCTION__, __LINE__);
  return 0;
}

int testErrno()
{
  EOS_SET("ERRNO_1");
  EOS_SET("ERRNO_2");
  EOS_SET("ERRNO_3");
  EOS_SET("ERRNO_4");

  return 0;
}

#endif
