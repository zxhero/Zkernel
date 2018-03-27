#include "common.h"
#include "syslib.h"
#include "util.h"
#include "printf.h"

void LiuBei(void)
{
	pid_t myPid = getpid();
  mbox_t pub = mbox_open("LiuBei-Publish-PID");
	pub=1;
  //printf(4,1, "LiuBei : my mubox     (%d)         ", pub);
//printf(5,1, "LIUBEI : my mubox     (%d)         ", pub);
  /* Send PID twice, once for sunquan Hood,
   * and once for the CaoCao
   */
	liubei_mux = 1;
   mutex_init(liubei_mux);
   mutex_acquire(liubei_mux);
  mbox_send(pub, &myPid, sizeof(pid_t));
  mbox_send(pub, &myPid, sizeof(pid_t));

  /* Find sunquan's PID */
  mbox_t sub = mbox_open("SunQuan-Publish-PID");
//printf(6,1, "LIUBEI : SUNQUAN mubox     (%d)         ", sub);
    pid_t aramis;
    mbox_recv(sub, &aramis, sizeof(pid_t));

    printf(3,1, "LiuBei(%d): I'll ask for suquan_mux         ", myPid);

    //wait(aramis);
	mutex_acquire(suquan_mux);

    printf(4,1, "LiuBei(%d): I have got the suquan_mux!", myPid);

    sleep(1000);
    //spawn("SunQuan");
    //mbox_send(pub, &myPid, sizeof(pid_t));
	exit();
}

