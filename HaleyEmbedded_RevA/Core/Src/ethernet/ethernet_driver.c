/*
 * ethernet_driver.c
 *
 * lwIP netconn-based TCP server for the Haley AI host communication link.
 *
 * The STM32 acts as the TCP server on HALEY_SERVER_PORT (7000).
 * The Linux AI host connects as a client.
 *
 * Design notes:
 *   - Uses lwIP netconn API (thread-safe, FreeRTOS-compatible).
 *   - One active connection at a time. Incoming connections while one is
 *     active will replace the previous connection after a graceful close.
 *   - Receive is exposed as a raw byte-stream function; the protocol layer
 *     (ethernet_protocol.c) handles packet reassembly and framing.
 *   - All netconn operations have explicit timeouts to avoid blocking tasks
 *     indefinitely on network issues.
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#include <string.h>
#include <stdio.h>

/* lwIP includes */
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/netbuf.h"
#include "lwip/sys.h"

/* FreeRTOS CMSIS-RTOS V2 */
#include "cmsis_os.h"

/* Project headers */
#include "ethernet/ethernet_driver.h"

/* -----------------------------------------------------------------------
 * Module-private state
 * ----------------------------------------------------------------------- */

static struct netconn *s_server_conn  = NULL;  /**< Listening server socket  */
static struct netconn *s_client_conn  = NULL;  /**< Active client connection  */

static eth_drv_state_t       s_state          = ETH_DRV_STATE_UNINIT;
static eth_drv_connect_cb_t  s_on_connect     = NULL;
static eth_drv_disconnect_cb_t s_on_disconnect = NULL;

/** Re-entrant protection for the send path (multiple tasks may call Send) */
static osMutexId_t s_send_mutex = NULL;
static const osMutexAttr_t s_send_mutex_attr = {
    .name      = "ethDrvSendMtx",
    .attr_bits = osMutexRecursive,
};

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/**
 * @brief  Close and free the active client connection, update state.
 */
static void close_client(void)
{
    if (s_client_conn != NULL) {
        netconn_close(s_client_conn);
        netconn_delete(s_client_conn);
        s_client_conn = NULL;
    }

    if (s_state == ETH_DRV_STATE_CONNECTED) {
        s_state = ETH_DRV_STATE_LISTENING;
        if (s_on_disconnect != NULL) {
            s_on_disconnect();
        }
    }
}

/* -----------------------------------------------------------------------
 * Public API Implementation
 * ----------------------------------------------------------------------- */

eth_drv_result_t ETH_DRV_Init(eth_drv_connect_cb_t    on_connect,
                               eth_drv_disconnect_cb_t on_disconnect)
{
    /* Guard against re-init without de-init */
    if (s_state != ETH_DRV_STATE_UNINIT) {
        return ETH_DRV_OK;
    }

    s_on_connect    = on_connect;
    s_on_disconnect = on_disconnect;
    s_state         = ETH_DRV_STATE_WAITING;

    /* Create send mutex */
    if (s_send_mutex == NULL) {
        s_send_mutex = osMutexNew(&s_send_mutex_attr);
        if (s_send_mutex == NULL) {
            s_state = ETH_DRV_STATE_ERROR;
            return ETH_DRV_ERR_LWIP;
        }
    }

    /* Create the TCP server netconn */
    s_server_conn = netconn_new(NETCONN_TCP);
    if (s_server_conn == NULL) {
        s_state = ETH_DRV_STATE_ERROR;
        return ETH_DRV_ERR_LWIP;
    }

    /* Allow address reuse so we can rebind quickly after a restart */
    netconn_set_recvtimeout(s_server_conn, HALEY_RECV_TIMEOUT_MS);

    /* Bind to all interfaces on HALEY_SERVER_PORT */
    err_t err = netconn_bind(s_server_conn, IP_ADDR_ANY, HALEY_SERVER_PORT);
    if (err != ERR_OK) {
        netconn_delete(s_server_conn);
        s_server_conn = NULL;
        s_state       = ETH_DRV_STATE_ERROR;
        return ETH_DRV_ERR_LWIP;
    }

    /* Start listening — backlog of 1 (we only service one client at a time) */
    err = netconn_listen_with_backlog(s_server_conn, 1);
    if (err != ERR_OK) {
        netconn_delete(s_server_conn);
        s_server_conn = NULL;
        s_state       = ETH_DRV_STATE_ERROR;
        return ETH_DRV_ERR_LWIP;
    }

    s_state = ETH_DRV_STATE_LISTENING;
    return ETH_DRV_OK;
}

/* ---------------------------------------------------------------------- */

eth_drv_result_t ETH_DRV_WaitForConnection(void)
{
    if (s_server_conn == NULL || s_state == ETH_DRV_STATE_UNINIT) {
        return ETH_DRV_ERR_NOT_INIT;
    }

    /* Close any stale client connection before accepting a new one */
    if (s_client_conn != NULL) {
        close_client();
    }

    s_state = ETH_DRV_STATE_LISTENING;

    /* Block until a client connects (or the server conn is closed) */
    struct netconn *new_conn = NULL;
    err_t err = netconn_accept(s_server_conn, &new_conn);

    if (err != ERR_OK || new_conn == NULL) {
        /* netconn_accept can return ERR_CLSD if the server is being torn down */
        return ETH_DRV_ERR_LWIP;
    }

    /* Configure timeouts on the client socket */
    netconn_set_recvtimeout(new_conn, HALEY_RECV_TIMEOUT_MS);
    netconn_set_sendtimeout(new_conn, HALEY_SEND_TIMEOUT_MS);

    s_client_conn = new_conn;
    s_state       = ETH_DRV_STATE_CONNECTED;

    /* Invoke connection callback with peer IP string */
    if (s_on_connect != NULL) {
        ip_addr_t peer_addr;
        uint16_t  peer_port = 0;
        netconn_peer(s_client_conn, &peer_addr, &peer_port);

        char ip_str[16] = {0};
        ipaddr_ntoa_r(&peer_addr, ip_str, sizeof(ip_str));
        s_on_connect(ip_str);
    }

    return ETH_DRV_OK;
}

/* ---------------------------------------------------------------------- */

eth_drv_result_t ETH_DRV_Receive(uint8_t *rx_buf,
                                  size_t   buf_size,
                                  size_t  *bytes_recvd)
{
    if (rx_buf == NULL || bytes_recvd == NULL || buf_size == 0) {
        return ETH_DRV_ERR_BUF;
    }

    *bytes_recvd = 0;

    if (s_client_conn == NULL || s_state != ETH_DRV_STATE_CONNECTED) {
        return ETH_DRV_ERR_NO_CONN;
    }

    struct netbuf *inbuf = NULL;
    err_t err = netconn_recv(s_client_conn, &inbuf);

    if (err == ERR_TIMEOUT) {
        return ETH_DRV_ERR_TIMEOUT;
    }

    if (err != ERR_OK || inbuf == NULL) {
        /* Connection closed or error */
        close_client();
        return ETH_DRV_ERR_NO_CONN;
    }

    /* Copy data from the netbuf chain into rx_buf */
    size_t total_copied = 0;

    do {
        void    *data_ptr  = NULL;
        uint16_t data_len  = 0;
        netbuf_data(inbuf, &data_ptr, &data_len);

        if (data_ptr == NULL || data_len == 0) {
            break;
        }

        size_t copy_len = data_len;
        if (total_copied + copy_len > buf_size) {
            /* Clip to available buffer space — caller must handle partial reads */
            copy_len = buf_size - total_copied;
        }

        memcpy(&rx_buf[total_copied], data_ptr, copy_len);
        total_copied += copy_len;

        if (total_copied >= buf_size) {
            break;  /* Buffer full; remaining data discarded this call */
        }

    } while (netbuf_next(inbuf) >= 0);

    netbuf_delete(inbuf);

    *bytes_recvd = total_copied;
    return ETH_DRV_OK;
}

/* ---------------------------------------------------------------------- */

eth_drv_result_t ETH_DRV_Send(const uint8_t *tx_buf, size_t tx_len)
{
    if (tx_buf == NULL || tx_len == 0) {
        return ETH_DRV_ERR_BUF;
    }

    if (s_client_conn == NULL || s_state != ETH_DRV_STATE_CONNECTED) {
        return ETH_DRV_ERR_NO_CONN;
    }

    /* Serialize concurrent senders */
    osMutexAcquire(s_send_mutex, osWaitForever);

    err_t err = netconn_write(s_client_conn,
                              tx_buf,
                              tx_len,
                              NETCONN_COPY);

    osMutexRelease(s_send_mutex);

    if (err != ERR_OK) {
        /* Mark as disconnected so the Rx task can reconnect */
        close_client();
        return ETH_DRV_ERR_SEND;
    }

    return ETH_DRV_OK;
}

/* ---------------------------------------------------------------------- */

eth_drv_result_t ETH_DRV_SendPacket(const uint8_t *pkt_buf, size_t pkt_len)
{
    return ETH_DRV_Send(pkt_buf, pkt_len);
}

/* ---------------------------------------------------------------------- */

void ETH_DRV_CloseConnection(void)
{
    close_client();
}

/* ---------------------------------------------------------------------- */

void ETH_DRV_DeInit(void)
{
    close_client();

    if (s_server_conn != NULL) {
        netconn_close(s_server_conn);
        netconn_delete(s_server_conn);
        s_server_conn = NULL;
    }

    if (s_send_mutex != NULL) {
        osMutexDelete(s_send_mutex);
        s_send_mutex = NULL;
    }

    s_state         = ETH_DRV_STATE_UNINIT;
    s_on_connect    = NULL;
    s_on_disconnect = NULL;
}

/* ---------------------------------------------------------------------- */

eth_drv_state_t ETH_DRV_GetState(void)
{
    return s_state;
}

/* ---------------------------------------------------------------------- */

int ETH_DRV_IsConnected(void)
{
    return (s_state == ETH_DRV_STATE_CONNECTED) ? 1 : 0;
}
