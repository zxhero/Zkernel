#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void SunQuan(void)
{
	pid_t myPid = getpid();
	
  mbox_t pub = mbox_open("SunQuan-Publish-PID");
  pub=0;
printf(3,1, "SunQuan : my mubox     (%d)         ", pub);
  /* Send PID twice, once for LiuBei,
   * and once for the CaoCao
   */
	//printf(1,1, "SunQuan (%d): I'm comming              ", myPid);
  mbox_send(pub, &myPid, sizeof(pid_t));
printf(1,1, "SunQuan (%d): I'm aa  comming              ", myPid);
  mbox_send(pub, &myPid, sizeof(pid_t));
//printf(2,1, "SunQuan (%d): I'm comming              ", myPid);
  /* Find LiuBei's PID */
  mbox_t sub = mbox_open("LiuBei-Publish-PID");
	//printf(4,1, "SunQuan : LIU_BEI mubox     (%d)         ", sub);
  for(;;)
  {
    pid_t liubei;
	
    mbox_recv(sub, &liubei, sizeof(pid_t));

    printf(1,1, "SunQuan (%d): I'm waiting for Liubei               ", myPid);

    wait(liubei);

    printf(1,1, "SunQuan(%d): I'm coming to save you, LiuBei!", myPid);

    sleep(1000);
    spawn("LiuBei");
    mbox_send(pub, &myPid, sizeof(pid_t));
  }
}
