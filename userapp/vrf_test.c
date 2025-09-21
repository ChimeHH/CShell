#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>

typedef unsigned int uint32_t;
#define IPPROTO_UDP 0
#define SO_VRF      0
#define SOCK_RAW SOCK_STREAM 

int DpoeNetworkDeviceSocket(uint32_t vrf_id, int reset_all)
{
#define NHASH_VCM_DEV_SOCK 8
typedef struct VCM_DEV_SOCK_TYPE{
  uint32_t vrf_id;
  int      sock;
} vCmDevSock_t;
  static vCmDevSock_t sock_list[NHASH_VCM_DEV_SOCK];

  if(reset_all == 0xca5aca5a) //a magic reset_all
  {
    int i;
    for(i=0; i<NHASH_VCM_DEV_SOCK; i++) 
    {
      printf("%d %08x %d\n", i, sock_list[i].vrf_id, sock_list[i].sock);
    }
    
    return 0;
  }
  
  if(reset_all)
  {
    int i;
    for(i=0; i<NHASH_VCM_DEV_SOCK; i++) 
    {
      sock_list[i].vrf_id = 0;
      if(sock_list[i].sock) close(sock_list[i].sock);
      sock_list[i].sock = 0;
    }
  }

  int num = NHASH_VCM_DEV_SOCK;
  int index = vrf_id % NHASH_VCM_DEV_SOCK;
  while(num-- > 0)
  {
    if(sock_list[index].sock <= 0) //not found, then create it
    {

      //int NetDeviceNewUDPv4Socket()
      int udpSock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
      if(udpSock == -1) 
      {
        perror("Failed to open udpSock");
        return -1;
      }
/*    
      if(vrf_id && setsockopt(udpSock, SOL_SOCKET, SO_VRF, &vrf_id, sizeof(vrf_id)) != 0) 
      {
        fprintf(stderr, "cm remote query polling socket "
                "setsockopt to reuse address failed, errno = %d, %s\n", errno, strerror(errno));

        return -1;
      }
*/
      sock_list[index].vrf_id = vrf_id;
      sock_list[index].sock = udpSock;
      return udpSock;
    }

    if(sock_list[index].vrf_id == vrf_id) //found
    {
      return sock_list[index].sock;
    }

    index += 1;
    if(index >= NHASH_VCM_DEV_SOCK) 
      index = 0;
  }

  return 0;
}