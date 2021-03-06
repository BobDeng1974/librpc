
#include  <client.h>
#include <msf_mempool.h>

#define MSF_MOD_DLNA "DLNA"
#define MSF_DLNA_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_DLNA, MSF_FUNC_FILE_LINE, __VA_ARGS__)

struct upnp_param_t {
    u8  upnp_nat;
    u8  upnp_discovery;
    u8  reserved[2];
    u8  friend_name[32];
}__attribute__((__packed__));

struct upnp_param_t upnp;

struct dlna_param_t    {
    u32 enable;
    u32 status;
    s8  confile[64];
    s8  friendly_name[32];
} __attribute__((__packed__));


s32 dlna_req_scb(s8 *data, u32 len, u32 cmd) {

    MSF_DLNA_LOG(DBG_DEBUG, 
        "DLNA req service callback data(%p) len(%d) cmd(%d).",
        data, len, cmd);

    if (DLNA_GET_PARAM == cmd) {
        memcpy(data, &upnp, sizeof(upnp));
    }
    return 0;
}

struct upnp_param_t upnp;

void upnp_debug_info(struct upnp_param_t *up) {
    MSF_DLNA_LOG(DBG_DEBUG, "UPnP friend name(%s).", up->friend_name);
    MSF_DLNA_LOG(DBG_DEBUG, "UPnP nat enable(%d).", up->upnp_nat);
    MSF_DLNA_LOG(DBG_DEBUG, "UPnP discovery enable(%d).", up->upnp_discovery);
}

s32 dlna_ack_scb(s8 *data, u32 len, u32 cmd) {

    MSF_DLNA_LOG(DBG_DEBUG, "DLNA ack service callback data(%p) len(%d) cmd(0x%x).",
                   data, len, cmd);

    switch (cmd) {
      case UPNP_GET_PARAM:
          if (data && len == sizeof(struct upnp_param_t)) {
              memcpy(&upnp, data, sizeof(upnp));
              upnp_debug_info(&upnp);
          }
          break;
      default:
          MSF_DLNA_LOG(DBG_ERROR, "Dlna unknown cmd(%u).", cmd);
          break;
    }

    return 0;
}

#define server_host "192.168.58.132"
#define SERVER_PORT "9999"

s32 main () {

    s32 rc = -1;
    s32 reg_mr = MSF_TRUE;
    
    struct msf_mempool *mempool = NULL;

    mempool = msf_mempool_create_prv(RPC_UPNP_ID, 
            (reg_mr ? MSF_MEMPOOL_FLAG_REG_MR : 0) |
            MSF_MEMPOOL_FLAG_HUGE_PAGES_ALLOC);

    if (!mempool) return -1;

    msf_mempool_dump(mempool);

    struct msf_reg_mem reg_mem;

    //msf_mem_alloc();

    msf_mempool_alloc(mempool, 8096, &reg_mem);
    msf_mempool_dump(mempool);
    msf_mempool_alloc(mempool, 8096, &reg_mem);
    msf_mempool_dump(mempool);
    msf_mempool_alloc(mempool, 8096, &reg_mem);


    msf_mempool_dump(mempool);

    msf_mempool_free(&reg_mem);

    msf_mempool_dump(mempool);

    msf_mempool_destroy(mempool);

    struct client_param param;
    param.name = MSF_MOD_DLNA;
    param.cid = RPC_DLNA_ID;
    param.host = LOCAL_HOST_V4;
    param.port = SERVER_PORT;
    param.req_scb = dlna_req_scb;
    param.ack_scb = dlna_ack_scb;

    rc = client_agent_init(&param);
    if (rc < 0) return -1;

    msf_memzero(&upnp, sizeof(struct upnp_param_t));

    struct basic_pdu pdu;
    msf_memzero(&pdu, sizeof(struct basic_pdu));
    pdu.dstid = RPC_UPNP_ID;
    pdu.cmd = UPNP_GET_PARAM;
    pdu.timeout = 5000;
    pdu.restload = (s8*)&upnp;
    pdu.restlen = sizeof(struct upnp_param_t);

    rc = client_agent_service(&pdu);

    if (rc != RPC_EXEC_SUCC) {
        MSF_DLNA_LOG(DBG_ERROR, "UPnP rpc call service errcode(%d).", rc);
    } else {
        MSF_DLNA_LOG(DBG_DEBUG, "UPnP friend name(%s).", upnp.friend_name);
        MSF_DLNA_LOG(DBG_DEBUG, "UPnP nat enable(%d).", upnp.upnp_nat);
        MSF_DLNA_LOG(DBG_DEBUG, "UPnP discovery enable(%d).", upnp.upnp_discovery);
    }

    while (1) {

        msf_memzero(&pdu, sizeof(struct basic_pdu));
        memcpy(upnp.friend_name, MSF_MOD_DLNA, strlen(MSF_MOD_DLNA));
        pdu.payload = (s8*)&upnp;
        pdu.paylen = sizeof(upnp);
        pdu.timeout = MSF_NO_WAIT;
        pdu.dstid = RPC_UPNP_ID;
        pdu.cmd = UPNP_SET_PARAM;
        rc = client_agent_service(&pdu);
        if (rc != RPC_EXEC_SUCC) {
           MSF_DLNA_LOG(DBG_ERROR, "UPnP rpc call service errcode(%d).", rc);
        } 
        msf_memzero(&pdu, sizeof(struct basic_pdu));
        pdu.timeout = MSF_NO_WAIT;
        pdu.cmd = UPNP_GET_PARAM;
        pdu.dstid = RPC_UPNP_ID;
        pdu.restlen = sizeof(struct upnp_param_t);
        rc = client_agent_service(&pdu);
        if (rc != RPC_EXEC_SUCC) {
           MSF_DLNA_LOG(DBG_ERROR, "UPnP rpc call service errcode(%d).", rc);
        }
        sleep(2);
    }

    client_agent_deinit();

    return 0;
}

