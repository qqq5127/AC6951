//#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "app_config.h"
#include "gma.h"
#include "generic/lbuf.h"
#include "init.h"
#include "gma_license.h"
#include "syscfg_id.h"
#include "vm.h"
#include "typedef.h"
#include "bt_tws.h"
#include "key_event_deal.h"

#if (GMA_EN)

#define GMA_DEBUG    printf
#define GMA_MALLOC()
#define GMA_FREE()

///manufactory:havit
#define ALI_PARA_ACTIVE_NUM               1
static const char *ali_para_string[] = {
    "jl_auif",
    //Product ID,Device Secret,Mac,Md5
    //此处可以用户自行填写从阿里申请的三元组信息
    " ",
};

ali_para_s ali_para;
volatile ali_para_s *active_ali_para = &ali_para;
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
ali_para_s ali_para_remote;
#endif
s32 vm_read(vm_hdl hdl, u8 *data_buf, u16 len);
s32 vm_write(vm_hdl hdl, u8 *data_buf, u16 len);

#if (GMA_EN == 2)
#define TEST_MODE_EN  			1///0:ali_para used "AUIF"  1:ali_para used ali_para_string
#else
#define TEST_MODE_EN  			0///0:ali_para used "AUIF"  1:ali_para used ali_para_string
#endif

static int gma_strcmp(const u8 *ptr1, const u8 *ptr2, u8 len)
{
    while (len--) {
        if (*ptr1 != *ptr2) {
            return (*ptr1 - *ptr2);
        }

        ptr1++;
        ptr2++;
    }

    return 0;
}
extern void lmp_hci_read_local_address(u8 *addr);
u8 bt_ble_adv_is_en(void);
void gma_adv_restore(void);
void gma_active_para_by_hci_para(u8 ble_reset)
{
    u8 mac_buf[6];
    lmp_hci_read_local_address(mac_buf);
    r_printf("gma adv mac: ");
    put_buf((const u8 *)mac_buf, 6);
    r_printf("gma local  mac: ");
    put_buf((const u8 *)ali_para.mac, 6);

    if (gma_strcmp(mac_buf, (const u8 *)ali_para.mac, 6) == 0) {
        printf("ali_para active \n");
        active_ali_para = &ali_para;
    }

#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    printf("gma remote mac: ");
    put_buf((const u8 *)ali_para_remote.mac, 6);
    if (gma_strcmp(mac_buf, (const u8 *)ali_para_remote.mac, 6) == 0) {
        printf("ali_para_remote active \n");
        active_ali_para = &ali_para_remote;
    }
#endif

    ///if adv active ,reflash adv
    if (bt_ble_adv_is_en() || ble_reset) {
        printf(">>>>>gma ble adv reflash \n");
        gma_adv_restore();
    }
}

void gma_active_local_para(void)
{
    u8 mac_buf[6];
    printf("gma local  mac: ");
    put_buf((const u8 *)ali_para.mac, 6);

    printf("ali_para active \n");
    active_ali_para = &ali_para;

    ///if adv active ,reflash adv
    if (bt_ble_adv_is_en()) {
        printf(">>>>>gma ble adv reflash \n");
        gma_adv_restore();
    }
}

bool gma_mac_is_match_active_ali_para(void)
{
    u8 mac_buf[6];
    if (active_ali_para == NULL) {
        printf(">>>>>>>>>active_ali_para null \n");
        return false;
    }

    lmp_hci_read_local_address(mac_buf);
    printf("gma adv mac: ");
    put_buf((const u8 *)mac_buf, 6);
    printf("gma active  mac: ");
    put_buf((const u8 *)active_ali_para->mac, 6);

    if (gma_strcmp(mac_buf, (const u8 *)active_ali_para->mac, 6) == 0) {
        printf("active_ali_para adv_mac match  \n");
        return true;
    }

    return false;
}


void *gma_get_ali_para(void)
{
    return (void *)active_ali_para;
}

/*
 *  mac address
 */
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
static volatile u8 used_remoted_mac_flg = 0;
#endif
///mac reverse
void gma_bt_mac_addr_get(uint8_t *buf)
{
    const u8 *mac_illegal = (const u8 *)"jl_mac";
    if (active_ali_para == NULL) {
        memcpy(buf, mac_illegal, 6);
        return ;
    }

    {
        ///check mac is legal
        int i;
        for (i = 0; i < 6; i++) {
            if ((active_ali_para->mac)[i]) {
                goto _get_mac;
            }
        }

        memcpy(buf, mac_illegal, 6);
        return ;
    }

_get_mac: {
        memcpy(buf, (const void *)active_ali_para->mac, 6);
    }
}

///mac unreverse
void gma_mac_addr_get(uint8_t *buf)
{
    ///get mac
    int i;
    for (i = 0; i < 6; i++) {
        buf[i] = (active_ali_para->mac)[5 - i];
    }
}

///mac reverse
void gma_mac_addr_reverse_get(uint8_t *buf)
{
    memcpy(buf, (const void *)active_ali_para->mac, 6);
}

///mac reverse
bool gma_sibling_mac_get(uint8_t *buf)
{
#if TCFG_USER_TWS_ENABLE
    if (get_tws_sibling_connect_state()) { //对耳已连接,需要同步提示音
        ///TWS HAD BEEN CONNECT
        ali_para_s *sibling_ali_para = NULL;//&ali_para;
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
        if (active_ali_para == &ali_para) {
            printf("ali_para active \n");
            sibling_ali_para = &ali_para_remote;
        }

        if (active_ali_para == &ali_para_remote) {
            printf("ali_para active \n");
            sibling_ali_para = &ali_para;
        }
#endif
        if (sibling_ali_para == NULL) {
            return false;
        }
        memcpy(buf, sibling_ali_para->mac, 6);
    } else {
        int i;
        u8 mac_buf[6];

        vm_read(VM_GMA_MAC, mac_buf, 6);
        if (gma_strcmp(mac_buf, (const u8 *)active_ali_para->mac, 6) == 0) {
            printf(">>>>>>>>>>>>>>>used ali_para mac \n");
            memcpy(mac_buf, ali_para.mac, 6);
        }

        memcpy(buf, mac_buf, 6);
    }
#endif

    return true;
}

u8 *gma_pid_get(void)
{
    return (u8 *)(&(active_ali_para->pid));
}

#if (GMA_USED_FIXED_TRI_PARA)
int gma_select_ali_para_by_mac(u8 *mac)
{
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    GMA_DEBUG("active mac:");
    put_buf(mac, 6);
    ///select para
    ali_para_s ali_para_temp;
    vm_read(VM_GMA_ALI_PARA, (u8 *) &ali_para_temp, sizeof(ali_para_s));
    if (ali_para_temp.pid == ali_para.pid) {
        memcpy(&ali_para_remote, &ali_para_temp, sizeof(ali_para_s));
        GMA_DEBUG("has remote mac \n");
        GMA_DEBUG("remote mac:");
        put_buf(ali_para_remote.mac, 6);
        GMA_DEBUG("remote pid:%d \n", ali_para_remote.pid);
        GMA_DEBUG("remote secret:");
        put_buf(ali_para_remote.secret, sizeof(ali_para_remote.secret));
        if (gma_strcmp(mac, ali_para_remote.mac, 6) == 0) {
            GMA_DEBUG("used remote para \n");
            active_ali_para = &ali_para_remote;
            return 0;
        } else if (gma_strcmp(mac, ali_para.mac, 6) == 0) {
            GMA_DEBUG("used local para \n");
            active_ali_para = &ali_para;
            return 0;
        } else {
            GMA_DEBUG("MAC ERROR !!!!!!!!!!!!!!!!\n");
            return -1;
        }
    } else {
        GMA_DEBUG("no remote mac \n");
    }
#endif
    return -1;
}
#endif

void tws_host_get_common_addr(/*u8 *local_addr,*/ u8 *remote_addr, u8 *common_addr, char channel)
{
    GMA_DEBUG(">>>>>>>>>tws_host_get_common_addr ch:%c \n", channel);

#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    used_remoted_mac_flg = 0;

    u8 mac_buf[6];
    gma_mac_addr_reverse_get(mac_buf);
    GMA_DEBUG(">>>local mac: ");
    put_buf(mac_buf, 6);
    ///used the bigger one as host bt mac
    if (gma_strcmp(mac_buf, (const u8 *)remote_addr, 6) < 0) {
        used_remoted_mac_flg = 1;
        ///used slave mac addr;remote_addr from slave addr reverse
        memcpy(common_addr, remote_addr, 6);
    } else {
        ///used master mac addr
        gma_mac_addr_reverse_get(common_addr);
    }
#else
    {
        gma_mac_addr_reverse_get(common_addr);
    }
#endif
    GMA_DEBUG("common_addr: ");
    put_buf((const u8 *)common_addr, 6);
    GMA_DEBUG("remote_addr: ");
    put_buf((const u8 *)remote_addr, 6);
    /* GMA_DEBUG("local_addr: "); */
    /* put_buf(mac_buf,6); */
}

void gma_send_secret_to_sibling(void)
{
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    void tws_data_to_sibling_send(u8 opcode, u8 * data, u8 len);;
    printf(">>>>>>>>>gma secret send \n");
    tws_data_to_sibling_send(TWS_AI_GMA_START_SYNC_LIC, (u8 *)(&(ali_para)), sizeof(ali_para));;
    //tws_data_to_sibling_send(TWS_AI_GMA_SYNC_LIC,NULL,0);;
    //used_remoted_mac_flg = 0;
#endif
}

void gma_send_secret_sync(void)
{
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    //gma_event_post(SYS_BT_AI_EVENT_TYPE_STATUS, KEY_GMA_SYNC_TRIPLE_SECRET);
#endif
}

void gma_kick_license_to_sibling(void)
{
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    void tws_data_to_sibling_send(u8 opcode, u8 * data, u8 len);;
    //if (used_remoted_mac_flg == 1)
    {
        tws_data_to_sibling_send(TWS_AI_GMA_SYNC_LIC, (u8 *) & (ali_para), sizeof(ali_para));;
    }
#endif
}

void gma_slave_sync_remote_addr(u8 *buf)
{
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    ///remote ali_para fill
    memcpy(&ali_para_remote, buf, sizeof(ali_para_remote));
    vm_write(VM_GMA_MAC, ali_para_remote.mac, 6);
#if (GMA_USED_FIXED_TRI_PARA)
    {
        ///write ali para to vm
        ali_para_s ali_para_temp;
        vm_read(VM_GMA_ALI_PARA, (u8 *) &ali_para_temp, sizeof(ali_para_remote));
        if (gma_strcmp((const u8 *)&ali_para_temp, (const u8 *)&ali_para_remote, sizeof(ali_para_remote)) != 0) {
            GMA_DEBUG("gma remote mac different \n");
            vm_write(VM_GMA_ALI_PARA, (u8 *)&ali_para_remote, sizeof(ali_para_remote));
        } else {
            GMA_DEBUG("gma remote ali para the same \n");
        }
    }
#endif
#endif
}

void gma_tws_remove_paired(void)
{
#if (GMA_USED_FIXED_TRI_PARA)
    ali_para_s ali_para_temp;
    memset(&ali_para_temp, 0xff, sizeof(ali_para_s));
    vm_write(VM_GMA_ALI_PARA, (u8 *)&ali_para_temp, sizeof(ali_para_s));
#endif
}

void gma_sync_remote_addr(u8 *buf)
{
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    ///remote ali_para fill
    memcpy(&ali_para_remote, buf, sizeof(ali_para_remote));
    vm_write(VM_GMA_MAC, ali_para_remote.mac, 6);
#if (GMA_USED_FIXED_TRI_PARA)
    {
        ///write ali para to vm
        ali_para_s ali_para_temp;
        vm_read(VM_GMA_ALI_PARA, (u8 *) &ali_para_temp, sizeof(ali_para_remote));
        if (gma_strcmp((const u8 *)&ali_para_temp, (const u8 *)&ali_para_remote, sizeof(ali_para_remote)) != 0) {
            GMA_DEBUG("gma remote mac different \n");
            vm_write(VM_GMA_ALI_PARA, (u8 *)&ali_para_remote, sizeof(ali_para_remote));
        } else {
            GMA_DEBUG("gma remote ali para the same \n");
        }
    }
#endif
    ///ble readv
    //gma_active_para_by_hci_para();
#if (GMA_BLE_ADV_MODE == 0)
    if (used_remoted_mac_flg == 1) {
        ///used slave address
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
        active_ali_para = &ali_para_remote;
#endif
        ble_mac_reset();
    }
#endif

#endif
}

void gma_adv_restore(void)
{
#if (GMA_TWS_PAIR_USED_FIXED_MAC)
    used_remoted_mac_flg = 0;
    //active_ali_para = &ali_para;
    ble_mac_reset();
#endif
}


///ble reset to local mac address:edr disconnect,unpair when edr unconnect

/*
 *  ali_para explanation.string format to array
 *  foramt of flash storage of ali_para(little endian):
 *  crc(4bytes) + content length(4bytes) + content
 */
extern u16 CRC16(const void *ptr, u32 len);
#define GMA_ALI_PARA_END_CHAR		','
#define GMA_ALI_PARA_MAC_CHAR_NUMS		(sizeof(((ali_para_s *)0)->mac))
#define GMA_ALI_PARA_PID_CHAR_NUMS		4//(sizeof(((ali_para_s *)0)->pid))
#define GMA_ALI_PARA_SECRET_CHAR_NUMS	(sizeof(((ali_para_s *)0)->secret))
#define GMA_AUTH_CODE_HEAD_LEN 		64

typedef struct __flash_of_ali_para_head {
    s16 crc;
    u16 string_len;
    const u8 para_string[];
} __attribute__((packed)) _flash_of_ali_para_head;

static bool gma_ali_para_head_check(const u8 *para)
{
    _flash_of_ali_para_head *head;

    //fill head
    head = (_flash_of_ali_para_head *)para;

    ///crc check
    u8 *crc_data = (u8 *)(para + sizeof(((_flash_of_ali_para_head *)0)->crc));
    u32 crc_len = sizeof(_flash_of_ali_para_head) - sizeof(((_flash_of_ali_para_head *)0)->crc)/*head crc*/ + (head->string_len)/*content crc,include end character '\0'*/;
    s16 crc_sum = 0;

    crc_sum = CRC16(crc_data, crc_len);

    if (crc_sum != head->crc) {
        GMA_DEBUG("gma crc error !!! %x %x \n", (u32)crc_sum, (u32)head->crc);
        return false;
    }

    return true;
}

/*common*/
static u8 gma_ali_para_char2num(u8 cha)
{
    u8 num = 0;

    if (cha >= '0' && cha <= '9') {
        num = cha - '0';
    } else if (cha >= 'a' && cha <= 'f') {
        num = cha - 'a' + 10;
    } else if (cha >= 'A' && cha <= 'F') {
        num = cha - 'A' + 10;
    }

    return num;
}
/*pid get*/
static bool gma_ali_para_pid_get(const u8 *para_string, ali_para_s *ali_para)
{
#define LEGAL_PID_NUM(cha) (cha>='0' && cha<='9')

    /*seek to pid*/
    NULL;

    /*get pid*/
    uint32_t pid = 0;
    uint32_t pid_char_nums = 0;
    while (*para_string != GMA_ALI_PARA_END_CHAR) {
        //if legal character
        if (!LEGAL_PID_NUM(*para_string)) {
            /* GMA_DEBUG("pid illegal character !!! \n"); */
            para_string++;
            continue;
///            return false;
        }

        ///check pid character numbers legal scope
        pid_char_nums++;
        if (pid_char_nums > GMA_ALI_PARA_PID_CHAR_NUMS) {
            GMA_DEBUG("pid character numbers overflow !!! \n");
            return false;
        }

        pid *= 10;
        pid += gma_ali_para_char2num(*para_string);
        para_string++;
    }

    ///check pid character numbers legal scope
    if (pid_char_nums != GMA_ALI_PARA_PID_CHAR_NUMS) {
        GMA_DEBUG("pid character numbers error:%d !!! \n", pid_char_nums);
        return false;
    }

    GMA_DEBUG(">>pid:%d \n", pid);
    ali_para->pid = pid;

    return true;
}
/*secret get*/
static bool gma_ali_para_secret_get(const u8 *para_string, ali_para_s *ali_para)
{
#define LEGAL_SECRET_NUM(cha) ((cha>='0'&&cha<='9') || (cha>='a'&&cha<='f') || (cha>='A'&&cha<='F'))
    /*seek to secret*/
    //leap over the pid string
    while (*para_string++ != GMA_ALI_PARA_END_CHAR) {
        NULL;
    }

    /*get secret*/
    memset(ali_para->secret, 0x00, sizeof(ali_para->secret));
    uint8_t *secret = ali_para->secret;
    u8 highbit_en = 0;
    uint32_t secret_char_nums = 0;
    while (*para_string != GMA_ALI_PARA_END_CHAR) {
        //if legal character
        if (!LEGAL_SECRET_NUM(*para_string)) {
            GMA_DEBUG("secret illegal character !!! \n");
            para_string++;
            continue;
//            return false;
        }

        ///assignment secret
        if (!highbit_en) {
            *secret += gma_ali_para_char2num(*para_string);
            highbit_en ^= 1;
        } else {
            ///check secret character numbers legal scope
            secret_char_nums++;
            if (secret_char_nums > GMA_ALI_PARA_SECRET_CHAR_NUMS) {
                GMA_DEBUG("secret character number overflow !!! \n");
                return false;
            }

            *secret <<= 4;
            *secret++ += gma_ali_para_char2num(*para_string);
            highbit_en ^= 1;
        }

        para_string++;
    }

    ///check secret character numbers legal scope
    if (secret_char_nums != GMA_ALI_PARA_SECRET_CHAR_NUMS) {
        GMA_DEBUG("secret character number error !!! :%d \n", secret_char_nums);
        return false;
    }

    ///print secret
    GMA_DEBUG(">>ali_para secret: ");
    for (int i = 0; i < sizeof(ali_para->secret) / sizeof(ali_para->secret[0]); i++) {
        GMA_DEBUG("%x ", ali_para->secret[i]);
    }

    return true;
}
/*mac get*/
static bool gma_ali_para_mac_get(const u8 *para_string, ali_para_s *ali_para)
{
#define LEGAL_MAC_NUM(cha) ((cha>='0'&&cha<='9') || (cha>='a'&&cha<='f') || (cha>='A'&&cha<='F') || (cha==':'))
#define USELESS_CHAR ':'
    /*seek to secret*/
    //leap over the pid string
    while (*para_string++ != GMA_ALI_PARA_END_CHAR) {
        NULL;
    }
    //leap over the secret string
    while (*para_string++ != GMA_ALI_PARA_END_CHAR) {
        NULL;
    }


    /*get mac*/
    memset(ali_para->mac, 0x00, sizeof(ali_para->mac));
    uint8_t *mac = ali_para->mac;
    u8 highbit_en = 0;
    uint32_t mac_char_nums = 0;
    while (*para_string != GMA_ALI_PARA_END_CHAR) {
        //if legal character
        if (!LEGAL_MAC_NUM(*para_string)) {
            GMA_DEBUG("mac illegal character !!! \n");
            para_string++;
            continue;
//            return false;
        }

        if (*para_string == USELESS_CHAR) {
            para_string++;
            continue;
        }

        ///assignment mac
        if (!highbit_en) {
            *mac += gma_ali_para_char2num(*para_string);
            highbit_en ^= 1;
        } else {
            //check mac character numbers legal scope
            mac_char_nums++;
            if (mac_char_nums > GMA_ALI_PARA_MAC_CHAR_NUMS) {
                GMA_DEBUG("mac character numbers overflow !!! \n");
                return false;
            }

            *mac <<= 4;
            *mac++ += gma_ali_para_char2num(*para_string);
            highbit_en ^= 1;
        }

        para_string++;
    }

    //check mac character numbers legal scope
    if (mac_char_nums != GMA_ALI_PARA_MAC_CHAR_NUMS) {
        GMA_DEBUG("mac character numbers error !!! \n");
        return false;
    }

    ///mac reverse
    for (int i = 0, tmp_mac = 0, array_size = (sizeof(ali_para->mac) / sizeof(ali_para->mac[0])); \
         i < (sizeof(ali_para->mac) / sizeof(ali_para->mac[0]) / 2); i++) {
        tmp_mac = ali_para->mac[i];
        ali_para->mac[i] = ali_para->mac[array_size - 1 - i];
        ali_para->mac[array_size - 1 - i] = tmp_mac;
    }

    ///print secret
    GMA_DEBUG(">>ali_para mac: ");
    for (int i = 0; i < sizeof(ali_para->mac) / sizeof(ali_para->mac[0]); i++) {
        GMA_DEBUG("%x ", ali_para->mac[i]);
    }


    return true;
}

static bool gma_ali_para_info_fill(const u8 *para, ali_para_s *ali_para)
{
    ///parameter string info dump
    //GMA_DEBUG(">>>>>>>>>>ALI PARA STRING:%s \n", para);

    if (gma_ali_para_pid_get(para, ali_para) == false) {
        GMA_DEBUG("get pid error !!! \n");
        return false;
    }

    if (gma_ali_para_secret_get(para, ali_para) == false) {
        GMA_DEBUG("get Secret error !!! \n");
        return false;
    }

    if (gma_ali_para_mac_get(para, ali_para) == false) {
        GMA_DEBUG("get mac error !!! \n");
        return false;
    }

    return true;
}

bool gma_ali_para_string2array(const u8 *para, ali_para_s *ali_para)
{
    _flash_of_ali_para_head *head;

    head = (_flash_of_ali_para_head *)para;

    if (gma_ali_para_head_check(para) == false) {
        GMA_DEBUG("gma_ali_para_head_check error !!!  \n");
        return false;
    }

    if (gma_ali_para_info_fill(head->para_string, ali_para) == false) {
        return false;
    }

    return true;
}

#if (USE_SDFILE_NEW == 1)
#define AUIF_PATH "mnt/sdfile/app/auif"
#else
#define AUIF_PATH "mnt/sdfile/C/RESERVED_CONFIG/AUIF"
#endif
//#include "sdfile.h"
#include "system/includes.h"
extern FILE *syscfg_file_open(const char *path);
extern void syscfg_file_close(FILE *fp);
static u32 gma_auth_addr = 0;
static int gma_license_get_addr_info(void)
{
    FILE *auif_fp = syscfg_file_open(AUIF_PATH);
    if (auif_fp == NULL) {
        return -1;
    }

    struct vfs_attr file_attr = {0};
    fget_attrs(auif_fp, &file_attr);
    syscfg_file_close(auif_fp);
    GMA_DEBUG(">>>>>>>>>>>gma license addr:%x  len:%x \n", file_attr.sclust, file_attr.fsize);

    return file_attr.sclust;
}

typedef enum _FLASH_ERASER {
    CHIP_ERASER = 0,
    BLOCK_ERASER,
    SECTOR_ERASER,
    PAGE_ERASER,
} FLASH_ERASER;
extern u32 sdfile_cpu_addr2flash_addr(u32 offset);
extern bool sfc_erase(FLASH_ERASER cmd, u32 addr);
extern u32 sfc_write(const u8 *buf, u32 addr, u32 len);
extern u32 sfc_read(u8 *buf, u32 addr, u32 len);
void gma_license_array2auif(const u8 *para)
{
    _flash_of_ali_para_head header;
    u8 buffer[256];
    u32 auif_addr = gma_license_get_addr_info() ? sdfile_cpu_addr2flash_addr(gma_license_get_addr_info()) : 0;
    GMA_DEBUG("auif flash addr:%x \n", auif_addr);
    if (auif_addr == 0) {
        return;
    }

    if (!gma_strcmp(para, (const u8 *)(gma_license_get_addr_info() + GMA_AUTH_CODE_HEAD_LEN + sizeof(_flash_of_ali_para_head)), 32)) {
        return;
    }

    sfc_erase(SECTOR_ERASER, auif_addr);

    if ((GMA_AUTH_CODE_HEAD_LEN + strlen((const char *)para)) > sizeof(buffer)) {
        GMA_DEBUG("para error !!!! LENGTH:%d \n", (int)strlen((const char *)para));
        return;
    }
    header.string_len = strlen((const char *)para) + 1;
    memset(buffer, 0xff, sizeof(buffer));
    ///length
    memcpy(&buffer[GMA_AUTH_CODE_HEAD_LEN + sizeof(header.crc)], &(header.string_len), sizeof(header.string_len));
    ///context
    memcpy(&buffer[GMA_AUTH_CODE_HEAD_LEN + sizeof(header.crc) + sizeof(header.string_len)], para, strlen((const char *)para));
    ///crc
    header.crc = CRC16(&buffer[GMA_AUTH_CODE_HEAD_LEN + sizeof(header.crc)], sizeof(header.string_len) + header.string_len);
    memcpy(&buffer[GMA_AUTH_CODE_HEAD_LEN], &(header.crc), sizeof(header.crc));

    sfc_write(buffer, auif_addr, 256);

}
////license_new_format
u32 gma_license_ptr(void)
{
    u32 flash_capacity = sdfile_get_disk_capacity();
    u32 auth_addr = flash_capacity - 256 + 80;
    GMA_DEBUG("flash capacity:%x \n", flash_capacity);
    return sdfile_flash_addr2cpu_addr(auth_addr);
}

static int gma_license_set(ali_para_s *ali_para)
{
    u8 *auth_ptr = (u8 *)gma_license_ptr();
    _flash_of_ali_para_head *head;

    put_buf(auth_ptr, 128);

    //head length check
    head = (_flash_of_ali_para_head *)auth_ptr;
    if (head->string_len >= 0xff) {
        GMA_DEBUG("gma license length error !!! \n");
        return (-1);
    }

    ////crc check
    if (gma_ali_para_head_check(auth_ptr) == (false)) {
        return (-1);
    }

    ///jump to context
    auth_ptr += sizeof(_flash_of_ali_para_head);
    ///license fill
    //pid
    memcpy((u8 *) & (ali_para->pid), auth_ptr, sizeof(ali_para->pid));
    auth_ptr += sizeof(ali_para->pid);
    GMA_DEBUG("gma pid:%d \n", ali_para->pid);
    //secret
    memcpy((u8 *)(ali_para->secret), auth_ptr, sizeof(ali_para->secret));
    auth_ptr += sizeof(ali_para->secret);
    GMA_DEBUG("gma secret:");
    put_buf(ali_para->secret, sizeof(ali_para->secret));
    //mac
    memcpy((u8 *)(ali_para->mac), auth_ptr, sizeof(ali_para->mac));
    auth_ptr += sizeof(ali_para->mac);
    {
        //mac reverse
        u8 mac_backup_buf[6];
        memcpy(mac_backup_buf, ali_para->mac, 6);
        int i;
        for (i = 0; i < 6; i++) {
            ali_para->mac[i] = (mac_backup_buf)[5 - i];
        }
    }
    GMA_DEBUG("gma mac:");
    put_buf(ali_para->mac, sizeof(ali_para->mac));

    return 0;
}

static int gma_license2flash(const ali_para_s *ali_para)
{
#define FORCE_TO_ERASE		    1
#define AUTH_PAGE_OFFSET 		80

    _flash_of_ali_para_head header;
    u8 buffer[256];
    u32 flash_capacity = sdfile_get_disk_capacity();
    u32 auth_sector = flash_capacity - 4 * 1024;
    u32 auth_page_addr = flash_capacity - 256;

    ///check if need update data to flash,erease auth erea if there are some informations in auth erea
    memset(buffer, 0xff, sizeof(buffer));
    if (!memcmp((u8 *)gma_license_ptr(), buffer, sizeof(ali_para_s) + sizeof(_flash_of_ali_para_head))) {
        ///context all 0xff

    } else {
        bool the_same_license = true;
        {
            ///check if ali_para the same with flash information
            u8 *auif_ptr = (u8 *)gma_license_ptr();
            auif_ptr += sizeof(_flash_of_ali_para_head);
            ///check pid
            if (memcmp(auif_ptr, (u8 *) & (ali_para->pid), sizeof(ali_para->pid))) {
                the_same_license = false;
            }
            auif_ptr += sizeof(ali_para->pid);
            ///check secret
            if (memcmp(auif_ptr, (u8 *)(ali_para->secret), sizeof(ali_para->secret))) {
                the_same_license = false;
            }
            auif_ptr += sizeof(ali_para->secret);
            ///check mac
            if (memcmp(auif_ptr, (u8 *)(ali_para->mac), sizeof(ali_para->mac))) {
                the_same_license = false;
            }
            auif_ptr += sizeof(ali_para->mac);

        }

        if (the_same_license == true) {
            GMA_DEBUG("flash information the same with ali_para \n");
            return 0;
        }

#if (!FORCE_TO_ERASE)
        GMA_DEBUG("flash information different with ali_para,but not supported flash erase !!! \n");
        return (-1);
#endif

        ///erase license sector
        {
            ///save last 256 byte
            GMA_DEBUG("gma erase license sector \n");
            memcpy(buffer, (u8 *)sdfile_flash_addr2cpu_addr(flash_capacity - 256), 256);
            sfc_erase(SECTOR_ERASER, auth_sector);
        }
    }

    {
        GMA_DEBUG("gma write license to flash \n");
        header.string_len = sizeof(ali_para_s);
        ///length
        memcpy(&buffer[AUTH_PAGE_OFFSET + sizeof(header.crc)], &(header.string_len), sizeof(header.string_len));
        ///context
        {
            //pid
            memcpy(&buffer[AUTH_PAGE_OFFSET + sizeof(header.crc) + sizeof(header.string_len)], \
                   (u8 *) & (ali_para->pid), sizeof(ali_para->pid));
            //secret
            memcpy(&buffer[AUTH_PAGE_OFFSET + sizeof(header.crc) + sizeof(header.string_len) + sizeof(ali_para->pid)], \
                   (u8 *)(ali_para->secret), sizeof(ali_para->secret));
            //mac
            u8 mac[6];
            {
                //mac reverse
                u8 mac_backup_buf[6];
                memcpy(mac_backup_buf, ali_para->mac, 6);
                int i;
                for (i = 0; i < 6; i++) {
                    mac[i] = (mac_backup_buf)[5 - i];
                }
            }
            memcpy(&buffer[AUTH_PAGE_OFFSET + sizeof(header.crc) + sizeof(header.string_len) + sizeof(ali_para->pid) + sizeof(ali_para->secret)], \
                   (u8 *)(mac), sizeof(ali_para->mac));
        }
        ///crc
        header.crc = CRC16(&buffer[AUTH_PAGE_OFFSET + sizeof(header.crc)], sizeof(header.string_len) + header.string_len);
        memcpy(&buffer[AUTH_PAGE_OFFSET], &(header.crc), sizeof(header.crc));

        sfc_write(buffer, auth_page_addr, 256);
    }

    return 0;
}

static u32 read_cfg_file(void *buf, u16 len, char *path)
{
    FILE *fp =  fopen(path, "r");
    int rlen;

    if (!fp) {
        printf("read_cfg_file:fopen err!\n");
        return FALSE;
    }

    rlen = fread(fp, buf, len);
    if (rlen <= 0) {
        printf("read_cfg_file:fread err!\n");
        return FALSE;
    }

    fclose(fp);

    return TRUE;
}


int gma_prev_init(void)
{
#if (!TEST_MODE_EN)
    return gma_license_set(&ali_para);
#else
///read license from flash file or array

    u8 *ali_str = ali_para_string[ALI_PARA_ACTIVE_NUM];
    u8 ali_license[128] = {0};

    //check if exist flash file
    if (read_cfg_file(ali_license, sizeof(ali_license), SDFILE_RES_ROOT_PATH"license.txt") == TRUE) {
        ali_str = ali_license;
    }
    r_printf("ali_str:%s\n", ali_str);
    if (gma_ali_para_info_fill((const u8 *)ali_str, &ali_para) == false) {
        GMA_DEBUG("array context not good \n");
///read license from flash
        return gma_license_set(&ali_para);
    }
///write license to flash
    gma_license2flash(&ali_para);
#endif
    return 0;
}

bool gma_ali_para_check(void)
{
#if (!TEST_MODE_EN)
    if (gma_auth_addr == 0) {
        GMA_DEBUG("app_use_flash_cfg.auif_addr error !!!! \n");
        return false;
    }

    const u8 *ali_para_string_obj = (const u8 *)(gma_auth_addr + GMA_AUTH_CODE_HEAD_LEN);
    if (gma_ali_para_string2array((const u8 *)ali_para_string_obj, \
                                  &ali_para) == false) {
        GMA_DEBUG("ali_para analisy error !!! \n");
        ;
        return false;
    }
#endif

    return true;
}

platform_initcall(gma_prev_init);
#endif//GMA_EN

