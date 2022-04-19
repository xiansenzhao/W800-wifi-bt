/**
 * @file    wm_udp_demo.c
 *
 * @brief   udp demo
 *
 * @author  dave
 *
 * Copyright (c) 2016 Winner Microelectronics Co., Ltd.
 */

#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "wm_sockets.h"

#if DEMO_UDP
#define DEMO_UDP_BROADCAST          0
#define DEMO_UDP_UNICAST                1
#define DEMO_UDP_MUTICAST               2

#define     DEMO_UDP_TASK_SIZE      512
#define     DEMO_UDP_BUF_SIZE            1024

#define DEMO_UDP_LOCAL_PORT         3000

static u8 MCASTIP[4] = {224, 1, 2, 1};

static OS_STK   udp_task_stk[DEMO_UDP_TASK_SIZE];
static OS_STK   udp_rcv_task_stk[DEMO_UDP_TASK_SIZE];


/**
 * @typedef struct demo_udp
 */
typedef struct demo_udp
{
    tls_os_queue_t *udp_q;
    struct ip_mreq mreq;
    char *sock_rx;
    char *sock_tx;
    int cast_mode;
    bool socket_ok;
    int socket_num;
    int port;
    u32 ip_addr;
    u32 rcv_data_len;
    int snd_skt_no;
    int snd_data_len;

} ST_Demo_Udp;

ST_Demo_Udp *demo_udp = NULL;

static void demo_udp_task(void *sdata);

void close_udp_socket_demo(void)
{
	printf("close udp socket \n");
	closesocket(demo_udp->socket_num);
}

int create_udp_socket_demo(void)
{
    struct sockaddr_in pin;
    struct tls_ethif *ethif;
    int ttl = 10;
    int loop = 0;

    ethif = tls_netif_get_ethif();
    printf("local ip : %d.%d.%d.%d\n",  ip4_addr1(ip_2_ip4(&ethif->ip_addr)), ip4_addr2(ip_2_ip4(&ethif->ip_addr)),
           ip4_addr3(ip_2_ip4(&ethif->ip_addr)), ip4_addr4(ip_2_ip4(&ethif->ip_addr)));
    demo_udp->socket_num = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&pin, 0, sizeof(struct sockaddr_in));
    pin.sin_family = AF_INET;               
    pin.sin_addr.s_addr = (u32)0x00000000UL;        //IPADDR_ANY
    pin.sin_port = htons(DEMO_UDP_LOCAL_PORT);

    printf("local port :%d\n", DEMO_UDP_LOCAL_PORT);
    if (bind(demo_udp->socket_num, (struct sockaddr *) &pin, sizeof(pin)) == -1)
    {
        printf("bind err\n");
        closesocket(demo_udp->socket_num);
        return WM_FAILED;
    }

    if (DEMO_UDP_MUTICAST == demo_udp->cast_mode)
    {
        //Configure multicast TTL
        printf("set muticast\n");
        if (setsockopt(demo_udp->socket_num, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
        {
            printf("IP_MULTICAST_TTL");
            closesocket(demo_udp->socket_num);
            return WM_FAILED;
        }
        //Local Loop setting
        if (setsockopt(demo_udp->socket_num, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
        {
            printf("IP_MULTICAST_LOOP");
            closesocket(demo_udp->socket_num);
            return WM_FAILED;
        }
        MEMCPY((char *) & (demo_udp->mreq.imr_multiaddr.s_addr), (char *)MCASTIP, 4);
        demo_udp->mreq.imr_interface.s_addr = htonl(0x00000000UL);
        //join muticast group
        if (setsockopt(demo_udp->socket_num, IPPROTO_IP, IP_ADD_MEMBERSHIP, &(demo_udp->mreq), sizeof(demo_udp->mreq)) < 0)
        {
            printf("IP_ADD_MEMBERSHIP");
            closesocket(demo_udp->socket_num);
            return WM_FAILED;
        }
    }

    demo_udp->socket_ok = TRUE;
    return WM_SUCCESS;
}


static void demo_udp_recv_task(void *sdata)
{
    ST_Demo_Udp *udp = (ST_Demo_Udp *)sdata;
    int ret = 0;
    struct sockaddr_in pin;
    socklen_t addrlen = sizeof(struct sockaddr);

    for(;;)
    {
        if (udp->socket_ok)
        {
            ret = recvfrom(udp->socket_num, udp->sock_rx, DEMO_UDP_BUF_SIZE,
                           0, (struct sockaddr *)&pin, &addrlen);

            if (ret <= DEMO_UDP_BUF_SIZE && ret > 0)
            {
                printf("rcv from %s  : port : %d len = %d rev_data: \n", inet_ntoa(pin.sin_addr), htons(pin.sin_port), ret);
				
				for(int ii = 0; ii < ret; ii++)
				{
					printf("%d ",udp->sock_rx[ii]);
				}printf("\n");
            }
            else
            {
                printf("rcv err\n");
                udp->socket_ok = FALSE;
                if (DEMO_UDP_MUTICAST == udp->cast_mode)
                {
                    setsockopt(udp->socket_num, IPPROTO_IP, IP_DROP_MEMBERSHIP, &(udp->mreq), sizeof(udp->mreq));
                }
                closesocket(udp->socket_num);
                printf("closesocket\n");
                tls_os_queue_send(udp->udp_q, (void *)DEMO_MSG_SOCKET_ERR, 0);
            }
            continue;
        }
        tls_os_time_delay(10);
    }
}

int udp_send_data_demo(int len)
{
    printf("\nlen=%d\n", len);
    if (NULL == demo_udp)
    {
        return WM_FAILED;
    }
    if (!demo_udp->socket_ok)
    {
        printf("skt not created\n");
        return WM_FAILED;
    }

    demo_udp->snd_data_len = len;
    tls_os_queue_send(demo_udp->udp_q, (void *)DEMO_MSG_UART_RECEIVE_DATA, 0);

    return WM_SUCCESS;
}




/**
 * @brief          udp demo
 *
 * @param[in]      cast_mode     unicast or broadcast, 0 is broadcast, 1 is unicast, default is broadcast
 * @param[in]      port                 port num
 * @param[in]      *ip                  if cast mode is unicast, need define ip addr.
 *
 * @retval         0     success
 * @retval         other failed
 *
 * @note           None
 */
int socket_udp_demo(int cast_mode, int port, char *ip)
{
    if (demo_udp)
    {
        printf("run\n");
        return WM_FAILED;
    }
    printf("udp demo,cast:%d, port:%d\n", cast_mode, port);
    if(-1 == cast_mode)
    {
        cast_mode = DEMO_UDP_BROADCAST;
    }
    if(-1 == port || port > 65536)
    {
        printf("port err\n");
        return WM_FAILED;
    }
    if(NULL == ip)
    {
        if (DEMO_UDP_UNICAST == cast_mode)
        {
            printf("donot input ip,will send broadcast packet\n");
            cast_mode = DEMO_UDP_BROADCAST;
        }
    }

    if (NULL == demo_udp)
    {
        demo_udp = tls_mem_alloc(sizeof(ST_Demo_Udp));
        if (NULL == demo_udp)
        {
            goto _error;
        }
        memset(demo_udp, 0, sizeof(ST_Demo_Udp));

        demo_udp->sock_rx = tls_mem_alloc(DEMO_UDP_BUF_SIZE + 1);
        if (NULL == demo_udp->sock_rx)
        {
            goto _error3;
        }

        demo_udp->sock_tx = tls_mem_alloc(DEMO_UDP_BUF_SIZE + 1);
        if (NULL == demo_udp->sock_tx)
        {
            goto _error4;
        }

        tls_os_queue_create(&(demo_udp->udp_q), DEMO_QUEUE_SIZE);
        //deal with socket's message
        tls_os_task_create(NULL, NULL,
                           demo_udp_task,
                           (void *)demo_udp,
                           (void *)udp_task_stk,          /* task's stack start address */
                           DEMO_UDP_TASK_SIZE*sizeof(u32),            /* task's stack size, unit:byte */
                           DEMO_UDP_TASK_PRIO,
                           0);

        //deal with socket's rx data
        tls_os_task_create(NULL, NULL,
                           demo_udp_recv_task,
                           (void *)demo_udp,
                           (void *)udp_rcv_task_stk,      /* task's stack start address */
                           DEMO_UDP_TASK_SIZE*sizeof(u32),            /* task's stack size, unit:byte */
                           DEMO_UDP_RECEIVE_TASK_PRIO,
                           0);
    }

    demo_udp->port = port;
    if (ip)
    {
        demo_udp->ip_addr = ipaddr_addr(ip);
    }
    demo_udp->cast_mode = cast_mode;

    return WM_SUCCESS;

_error4:
    tls_mem_free(demo_udp->sock_rx);
_error3:
    tls_mem_free(demo_udp);
    demo_udp = NULL;
_error:
    return WM_FAILED;
}


static void udp_net_status_changed_event(u8 status )
{
    switch(status)
    {
    case NETIF_WIFI_JOIN_FAILED:
		printf("send DEMO_MSG_WJOIN_FAILD \n");
        tls_os_queue_send(demo_udp->udp_q, (void *)DEMO_MSG_WJOIN_FAILD, 0);
        break;
    case NETIF_WIFI_JOIN_SUCCESS:
		printf("send DEMO_MSG_WJOIN_SUCCESS \n");
        tls_os_queue_send(demo_udp->udp_q, (void *)DEMO_MSG_WJOIN_SUCCESS, 0);
        break;
    case NETIF_IP_NET_UP:
		printf("send DEMO_MSG_SOCKET_CREATE \n");
        tls_os_queue_send(demo_udp->udp_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
        break;
    default:
        break;
    }
}

static void demo_udp_task(void *sdata)
{
    ST_Demo_Udp *udp = (ST_Demo_Udp *)sdata;
    void *msg;
    struct tls_ethif *ethif = tls_netif_get_ethif();
    int len;
    int ret;
    struct sockaddr_in pin;

    if(ethif->status)	/*connected to ap and get IP*/
    {
        tls_os_queue_send(udp->udp_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
    }
    else
    {
        struct tls_param_ip ip_param;

        tls_param_get(TLS_PARAM_ID_IP, &ip_param, TRUE);
        ip_param.dhcp_enable = TRUE;
        tls_param_set(TLS_PARAM_ID_IP, &ip_param, TRUE);
        tls_wifi_set_oneshot_flag(1);		 /*Enable oneshot configuration*/
        printf("\nwait one shot......\n");
    }
    tls_netif_add_status_event(udp_net_status_changed_event);

    memset(&pin, 0, sizeof(struct sockaddr_in));
    pin.sin_family = AF_INET;              

    for (;;)
    {
        tls_os_queue_receive(udp->udp_q, (void **)&msg, 0, 0);
        //printf("\n udp msg =%d\n",msg);
        switch((u32)msg)
        {
        case DEMO_MSG_WJOIN_SUCCESS:
            break;

        case DEMO_MSG_SOCKET_CREATE:
            create_udp_socket_demo();
            break;

        case DEMO_MSG_WJOIN_FAILD:
            if(udp->socket_num > 0)
            {
                udp->socket_num = 0;
                udp->socket_ok = FALSE;
            }
            break;

        case DEMO_MSG_SOCKET_RECEIVE_DATA:
            break;

        case DEMO_MSG_UART_RECEIVE_DATA:
            if (-1 == udp->snd_data_len)
            {
                len = DEMO_UDP_BUF_SIZE;
            }
            else if(udp->snd_data_len != 0)
            {
                len = (udp->snd_data_len > DEMO_UDP_BUF_SIZE) ?
                      DEMO_UDP_BUF_SIZE : udp->snd_data_len;
            }
            else
            {
                break;
            }
            //memset(udp->sock_tx, 'u', len);

            if (DEMO_UDP_BROADCAST == udp->cast_mode)
            {
                pin.sin_addr.s_addr = htonl(0xffffffffUL);  //IPADDR_BROADCAST
            }
            else if (DEMO_UDP_MUTICAST == udp->cast_mode)
            {
                MEMCPY((char *) & (pin.sin_addr.s_addr), (char *)MCASTIP, 4);
            }
            else
            {
                pin.sin_addr.s_addr = udp->ip_addr;
            }

            pin.sin_port = htons(udp->port);

            ret = sendto(udp->socket_num, udp->sock_tx, len, 0, (struct sockaddr *)&pin, sizeof(struct sockaddr));
			printf("sendto ret = %d\n",ret);
            if (ret < 0)
            {
                printf("send err\n");
                break;
            }
            else
            {
                if (udp->snd_data_len != -1)
                {
                    udp->snd_data_len -= ret;
                }
            }
            if (udp->socket_ok && udp->snd_data_len != 0)
            {
                tls_os_time_delay(8);
                tls_os_queue_send(udp->udp_q, (void *)DEMO_MSG_UART_RECEIVE_DATA, 0);
            }
			
			printf("send_len: %d data:\n",len);
			for(int ii = 0;ii < len; ii++)
			{
				printf("%x ",udp->sock_tx[ii]);
			}printf("\n");
			
			
            break;

        case DEMO_MSG_SOCKET_ERR:
            tls_os_time_delay(200);
            printf("\nsocket err\n");
            create_udp_socket_demo( );
            break;

        default:
            break;
        }
    }

}

//add by zxx start
#define VERSION_3           3
#define VERSION_4           4

#define MODE_CLIENT         3
#define MODE_SERVER         4
#define NTP_LI              0
#define NTP_VN              VERSION_3   
#define NTP_MODE            MODE_CLIENT
#define NTP_STRATUM         0
#define NTP_POLL            4
#define NTP_PRECISION       -6

#define NTP_HLEN            48

#define BUFSIZE             1500

#define JAN_1970            0x83aa7e80

#define NTP_CONV_FRAC32(x)  (uint64_t) ((x) * ((uint64_t)1<<32))    
#define NTP_REVE_FRAC32(x)  ((double) ((double) (x) / ((uint64_t)1<<32)))   

#define NTP_CONV_FRAC16(x)  (uint32_t) ((x) * ((uint32_t)1<<16))    
#define NTP_REVE_FRAC16(x)  ((double)((double) (x) / ((uint32_t)1<<16)))    


#define USEC2FRAC(x)        ((uint32_t) NTP_CONV_FRAC32( (x) / 1000000.0 )) 
#define FRAC2USEC(x)        ((uint32_t) NTP_REVE_FRAC32( (x) * 1000000.0 )) 


#define NTP_LFIXED2DOUBLE(x)    ((double) ( ntohl(((struct l_fixedpt *) (x))->intpart) - JAN_1970 + FRAC2USEC(ntohl(((struct l_fixedpt *) (x))->fracpart)) / 1000000.0 ))   


struct s_fixedpt {
    uint16_t    intpart;
    uint16_t    fracpart;
};

struct l_fixedpt {
    uint32_t    intpart;
    uint32_t    fracpart;
};
struct ntphdr {
#if __BYTE_ORDER == __BID_ENDIAN
    unsigned int    ntp_li:2;
    unsigned int    ntp_vn:3;
    unsigned int    ntp_mode:3;
#endif
/*
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int    ntp_mode:3;
    unsigned int    ntp_vn:3;
    unsigned int    ntp_li:2;
#endif
*/
    uint8_t         ntp_stratum;
    uint8_t         ntp_poll;
    int8_t          ntp_precision;
    struct s_fixedpt    ntp_rtdelay;
    struct s_fixedpt    ntp_rtdispersion;
    uint32_t            ntp_refid;
    struct l_fixedpt    ntp_refts;
    struct l_fixedpt    ntp_orits;
    struct l_fixedpt    ntp_recvts;
    struct l_fixedpt    ntp_transts;
};

#include <time.h>

int get_ntp_packet(void *buf, size_t *size)
{
    struct ntphdr *ntp;
    struct timeval tv;


    if (!size || *size<NTP_HLEN)
        return -1;

    memset(buf, 0, *size);

    ntp = (struct ntphdr *) buf;
    ntp->ntp_li = NTP_LI;
    ntp->ntp_vn = NTP_VN;
    ntp->ntp_mode = NTP_MODE;
    ntp->ntp_stratum = NTP_STRATUM;
    ntp->ntp_poll = NTP_POLL;
    ntp->ntp_precision = NTP_PRECISION;

    //gettimeofday(&tv, NULL);
    //ntp->ntp_transts.intpart = htonl(tv.tv_sec + JAN_1970);
    //ntp->ntp_transts.fracpart = htonl(USEC2FRAC(tv.tv_usec));
	
	ntp->ntp_transts.intpart = htonl(0 + JAN_1970);
    ntp->ntp_transts.fracpart = htonl(USEC2FRAC(0));

    *size = NTP_HLEN;

    return 0;
}

void print_ntp(struct ntphdr *ntp)
{
    time_t time;

    printf("LI:\t%d \n", ntp->ntp_li);
    printf("VN:\t%d \n", ntp->ntp_vn);
    printf("Mode:\t%d \n", ntp->ntp_mode);
    printf("Stratum:\t%d \n", ntp->ntp_stratum);
    printf("Poll:\t%d \n", ntp->ntp_poll);
    printf("precision:\t%d \n", ntp->ntp_precision);

    printf("Route delay:\t %lf \n",
        ntohs(ntp->ntp_rtdelay.intpart) + NTP_REVE_FRAC16(ntohs(ntp->ntp_rtdelay.fracpart)));
    printf("Route Dispersion:\t%lf \n",
        ntohs(ntp->ntp_rtdispersion.intpart) + NTP_REVE_FRAC16(ntohs(ntp->ntp_rtdispersion.fracpart)));
    printf("Referencd ID:\t %d \n", ntohl(ntp->ntp_refid));


    time = ntohl(ntp->ntp_refts.intpart) - JAN_1970;
    printf("Reference:\t%d %ld %s \n",
        ntohl(ntp->ntp_refts.intpart) - JAN_1970,
        FRAC2USEC(ntohl(ntp->ntp_refts.fracpart)),
        ctime(&time));

    time = ntohl(ntp->ntp_orits.intpart) - JAN_1970;
    printf("Originate:\t%d %d frac=%ld (%s) \n",
        ntohl(ntp->ntp_orits.intpart) - JAN_1970,
        FRAC2USEC(ntohl(ntp->ntp_orits.fracpart)),
        ntohl(ntp->ntp_orits.fracpart),
        ctime(&time) );

    time = ntohl(ntp->ntp_recvts.intpart) - JAN_1970;
    printf("Receive:\t%d %d (%s) \n",
        ntohl(ntp->ntp_recvts.intpart) - JAN_1970,
        FRAC2USEC(ntohl(ntp->ntp_recvts.fracpart)),
        ctime(&time) );

    time = ntohl(ntp->ntp_transts.intpart) - JAN_1970;
    printf("Transmit:\t%d %d (%s) \n",
        ntohl(ntp->ntp_transts.intpart) - JAN_1970,
        FRAC2USEC(ntohl(ntp->ntp_transts.fracpart)),
        ctime(&time) );
}
double get_rrt(const struct ntphdr *ntp, const struct timeval *recvtv)
{
    double t1, t2, t3, t4;

    t1 = NTP_LFIXED2DOUBLE(&ntp->ntp_orits);
    t2 = NTP_LFIXED2DOUBLE(&ntp->ntp_recvts);
    t3 = NTP_LFIXED2DOUBLE(&ntp->ntp_transts);
    t4 = recvtv->tv_sec + recvtv->tv_usec / 1000000.0;

    return (t4 - t1) - (t3 - t2);
}


double get_offset(const struct ntphdr *ntp, const struct timeval *recvtv)
{
    double t1, t2, t3, t4;

    t1 = NTP_LFIXED2DOUBLE(&ntp->ntp_orits);
    t2 = NTP_LFIXED2DOUBLE(&ntp->ntp_recvts);
    t3 = NTP_LFIXED2DOUBLE(&ntp->ntp_transts);
    t4 = recvtv->tv_sec + recvtv->tv_usec / 1000000.0;

    return ((t2 - t1) + (t3 - t4)) / 2;
}



void udp_send_data_self(u8 *data,int data_len)
{
	
	memcpy(demo_udp->sock_tx, data, data_len);
	udp_send_data_demo(data_len);
}

//add by zxx end



#endif
