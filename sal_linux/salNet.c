#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#include "zType_Def.h"
#include "zSalOS.h"
#include "zTraceApi.h"

//returns 0 on error
dword_t zIPAddr(const char *cp)
{
  if(!cp) return -1;
  in_addr_t r = inet_addr(cp);
  
  if(r == INADDR_NONE) return 0;
  
  return ntohl(r);
}
//returns 0 on error
dword_t zInetAddr(const char *cp)
{
  if(!cp) return -1;
  in_addr_t r = inet_addr(cp);
  
  if(r == INADDR_NONE) return 0;
  
  return (r);
}

byte_t *zNewMac(dword_t m1, dword_t m2)
{
  //zTraceP("input %04x, %08x\n", m1, m2);
  byte_t *mac = (byte_t *)malloc(6);
  mac[0] = (m1 >> 8) & 0xff;
  mac[1] = (m1) & 0xff;
  mac[2] = (m2 >> 24) & 0xff;
  mac[3] = (m2 >> 16) & 0xff;
  mac[4] = (m2 >> 8) & 0xff;
  mac[5] = (m2 ) & 0xff;

  //zTraceP("%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return mac;
}


int zCreateSocketEx(word_t lport, const char* function, int line)
{
  struct sockaddr_in  saddr; 
  int sock;
  
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock<0)
  {
    zTraceAlarm("lc data socket @port %d error: %s -- @%s, %d\n", lport, strerror(errno), function, line);
    return -1;
  }
  
  memset(&saddr,0,sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(lport);
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sock,(struct sockaddr *)&saddr, sizeof (saddr))!=0)	
  {
    zTraceAlarm("bind socket @port %d failed: %s! -- @%s, %d\n", lport, strerror(errno), function, line);
    return -1;
  }

  return sock;
}

int zSendtoEx(int sock, dword_t rip, word_t rport, byte_t *buf, int len, const char* function, int line)
{
  int rc;
  struct sockaddr_in  saddr; 
  
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(rip);
  saddr.sin_port = htons(rport); 

  rc = sendto(sock, buf, len, 0, (const struct sockaddr*)(&saddr), sizeof(saddr));
  if(rc < 0)
  {
    zTraceError("Sendto @sock %d remote %08x:%d, len %d failed: %s! -- @%s, %d\n", sock, rip, rport, len, strerror(errno), function, line);
    return -1;
  }
  //zTraceDebug("Sendto @sock %d remote %08x:%d, len %d returned: %d -- @%s, %d\n", sock, rip, rport, len, rc, function, line);
    
  return rc;
}

int zTryRecvfromEx(int sock, dword_t *rip, word_t *rport, byte_t *buf, int len, int sec, int usec, const char* function, int line)
{
  int rc;
  
  fd_set rfds;
  struct timeval tv;

  struct sockaddr_in  saddr; 
  int sin_len = sizeof(saddr);
  
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  
  if(!sec && !usec)
  {
    tv.tv_sec = 0;
    tv.tv_usec = 500;
  }
  else
  {
    tv.tv_sec = sec;
    tv.tv_usec = usec;
  }
  
  rc = select(sock+1, &rfds, NULL, NULL, &tv);  
  if (rc < 0)
  {
    zTraceError("Recvfrom @sock %d failed: %s! -- @%s, %d\n", sock, strerror(errno), function, line);
    return -1;
  }
  
  if (rc)
  {
    if(FD_ISSET(sock, &rfds))
    {
      rc = recvfrom(sock, buf, len, 0, (struct sockaddr *)&saddr, (dword_t *)&sin_len);
      if(rc > 0)
      {
        if(rip) *rip = ntohl(saddr.sin_addr.s_addr);
        if(rport) *rport = ntohs(saddr.sin_port);

        zTraceDebug("TryReceive @sock %d remote %08x:%d, len %d returned: %d -- @%s, %d\n", sock, 
                     ntohl(saddr.sin_addr.s_addr), ntohs(saddr.sin_port), len, rc, function, line);
  	    return rc;
  	  }
    }
  }

  return 0;
}


#define I_FD_S(nx, fds)            (nx ? fds : 0)
#define I_FD_ZERO(nx, fds)   do { if(nx) FD_ZERO(fds); } while(0)
#define I_FD_SET(nx, sock, fds)    do { if(nx) FD_SET(sock, fds);  } while(0)
#define I_FD_ISSET(nx, sock, fds)  (nx && FD_ISSET(sock, fds))

int zEnumateSocketEx(int sec, int usec, zSockEx_t ex[], int count, int *nread, int *nwrite, int *nexception, const char* function, int line)
{
  int rc, i, mx;
  
  fd_set rfds, wfds, efds;
  
  struct timeval tv;  
  tv.tv_sec = sec;
  tv.tv_usec = usec;

  mx = 0;
  I_FD_ZERO(nread,  &rfds);
  I_FD_ZERO(nwrite, &wfds);
  I_FD_ZERO(nexception, &efds);
  for(i=0; i<count; i++)
  {
    mx = MAX(ex[i].sock, mx);
    I_FD_SET(nread, ex[i].sock, &rfds);
    I_FD_SET(nwrite, ex[i].sock, &wfds);
    I_FD_SET(nexception, ex[i].sock, &efds);
  }
  
  rc = select(mx+1, &rfds, &wfds, &efds, &tv);
  if(rc == 0) return 0;
  
  if (rc < 0)
  {
    zTraceError("select failed: %s! -- @%s, %d\n", strerror(errno), function, line);
    return -errno;
  }
  
  int nr = 0; 
  int nw = 0;
  int ne = 0;
  for(i=0; i<count; i++)
  {
    if(I_FD_ISSET(nread, ex[i].sock, &rfds))      { ex[i].readable = 1; nr ++; } else { ex[i].readable = 0; };
    if(I_FD_ISSET(nwrite, ex[i].sock, &wfds))     { ex[i].writable = 1; nw ++; } else { ex[i].writable = 0; };
    if(I_FD_ISSET(nexception, ex[i].sock, &efds)) { ex[i].exception= 1; ne ++; } else { ex[i].exception= 0; };
  }
  if(nread) *nread = nr;
  if(nwrite) *nwrite = nw;
  if(nexception) *nexception = ne;
  
  return rc;
}

int zCanReadSocketEx(int sock, const char* function, int line)
{ 
  fd_set rfds;  
  static struct timeval tv = {0, 0};

  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  
  int rc = select(sock+1, &rfds, 0, 0, &tv);
   
  return rc;
}

int zCanWriteSocketEx(int sock, const char* function, int line)
{ 
  fd_set wfds;  
  static struct timeval tv = {0, 0};

  FD_ZERO(&wfds);
  FD_SET(sock, &wfds);
  
  int rc = select(sock+1, 0, &wfds, 0, &tv);
   
  return rc;
}


int zRecvfromEx(int sock, dword_t *rip, word_t *rport, byte_t *buf, int len, const char* function, int line)
{
  int rc;  
 
  struct sockaddr_in  saddr; 
  int sin_len = sizeof(saddr);
    
  rc = recvfrom(sock, buf, len, MSG_DONTWAIT, (struct sockaddr *)&saddr, (dword_t *)&sin_len);
  if(rc == 0) return 0;
  
  if (rc < 0)
  {
    if(errno == EAGAIN) return 0; //not a proper manner. to be implemented later.
    zTraceError("recvfrom failed @sock %d: %s! -- @%s, %d\n", sock, strerror(errno), function, line);
    return -errno;
  }

  if(rip) *rip = ntohl(saddr.sin_addr.s_addr);
  if(rport) *rport = ntohs(saddr.sin_port);

  //zTraceDebug("Receive @sock %d remote %08x:%d, len %d returned: %d -- @%s, %d\n", sock, 
  //                   ntohl(saddr.sin_addr.s_addr), ntohs(saddr.sin_port), len, rc, function, line);

  return rc;
}


int zShutdownSocketEx(int sock, const char* function, int line)
{
  shutdown(sock, SHUT_RDWR);  
  zTraceAlarm("shutdown sock %d  -- @%s, %d\n", sock, function, line);
  
  return 0;
}

int zCloseSocketEx(int sock, const char* function, int line)
{
  close(sock);
  zTraceAlarm("shutdown sock %d  -- @%s, %d\n", sock, function, line);
  
  return 0;
}

int zSetNonBlockSocket(int sock, const char* function, int line)
{
  if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0)|O_NONBLOCK) == -1)
  {
    zTraceAlarm("set non block failed, sock %d  -- @%s, %d\n", sock, function, line);
    return -1;
  }
  
  return 0;
}


int zSleep(int sec)
{
  sleep(sec);
  
  return 0;
}

int zSleepUSec(int usec)
{
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = usec;

  select(0, NULL, NULL, NULL, &tv); 
  
  return 0;
}

#include <time.h>

int zTime()
{
  return (int)time(0);
}

char *zCTime(int *pT)
{
  time_t t = (time_t)(*pT);
  return ctime(&t);
}


#if 1 //SALNET_SELFTEST

//zCreateSocket(port, __FUNCTION__, __LINE__);
//zCloseSocket(sock, __FUNCTION__, __LINE__);
//zSetNonBlockSocket(sock, __FUNCTION__, __LINE__);

int testNbLoop = 1;
int testNbLoopMax = 30;
int testNbSocket(int sock0, int sock1, word_t port1)
{
  char* str = "Hello, from 1025";
  byte_t buf[100];

  int rc;  

  while((testNbLoop < testNbLoopMax) && (testNbLoop++ > 0))
  {      
    if((testNbLoop % 5) == 0)
    {
      rc = zSendto(sock0, 0x7F000001, port1, (byte_t*)str, strlen(str)+1);
      zTraceP("%d send to, rc=%d\n", testNbLoop, rc);      

      if(rc < 0)  zTraceP("error: %d, %s\n", errno, strerror(errno));
    }
    
    dword_t rip = 0;
    word_t rport = 0;
    rc = zRecvfromEx(sock1, &rip, &rport, buf, 100, __FUNCTION__, __LINE__);
    zTraceP("%d recv from %08x %d, rc=%d\n", testNbLoop, rip, rport, rc);

    if(rc < 0)  zTraceP("error: %d, %s\n", errno, strerror(errno));

    zSleep(1);
  }
  
  return 0;
}

int testSocketSend(int sock, dword_t rip, word_t rport, const char* str)
{
  if(!str) return -1;

  if(!rip) rip = 0x7F000001;

  int rc = zSendto(sock, rip, rport, (byte_t*)str, strlen(str)+1);
  return rc;
}

int testSocketRecv(int sock)
{
  int rc = zCanReadSocketEx(sock, __FUNCTION__, __LINE__);  
  if(rc <= 0)
  {
    if(rc < 0) zTraceP("not readable, error: %d, %s\n", errno, strerror(errno));
    zTraceP("not readable: %d\n", rc);
    return rc;
  }
  
  dword_t rip = 0;
  word_t rport = 0;
  byte_t buf[100];
  rc = zRecvfromEx(sock, &rip, &rport, buf, 100, __FUNCTION__, __LINE__);
  zTraceP("recv from %08x %d, rc=%d\n", rip, rport, rc);

  if(rc < 0)  zTraceP("error: %d, %s\n", errno, strerror(errno));
  return rc;
}

#endif /*SALNET_SELFTEST*/

