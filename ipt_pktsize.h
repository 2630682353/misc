#ifndef __IPT_PKTSIZE_H
#define __IPT_PKTSIZE_H

#define PKTSIZE_VERSION "0.1"

struct ipt_pktsize_info {
    u_int32_t min_pktsize,max_pktsize;
};

#endif //__IPT_EXLENGTH_H
