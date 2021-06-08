#ifndef _GMA_LICENSE_H
#define _GMA_LICENSE_H

typedef struct {
    ///reverse
    uint8_t mac[6];
    uint32_t pid;
    uint8_t secret[16];
} ali_para_s;

void gma_bt_mac_addr_get(uint8_t *buf);
void gma_mac_addr_get(uint8_t *buf);
void gma_mac_addr_reverse_get(uint8_t *buf);
void gma_active_para_by_hci_para(u8 ble_reset);
void gma_active_local_para(void);
void gma_slave_sync_remote_addr(u8 *buf);
bool gma_sibling_mac_get(uint8_t *buf);
void gma_send_secret_to_sibling(void);
#if (GMA_USED_FIXED_TRI_PARA)
int gma_select_ali_para_by_mac(u8 *mac);
void gma_tws_remove_paired(void);
#endif
#endif
