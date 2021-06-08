#include "effect_tool.h"
#include "system/includes.h"
#include "media/includes.h"
#include "config/config_interface.h"
#include "app_config.h"
#include "app_online_cfg.h"
#include "application/audio_eq.h"

#define LOG_TAG     "[APP-EFFECTS]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#if (TCFG_MIC_EFFECT_ENABLE)

#if TCFG_MIC_EFFECT_ONLINE_ENABLE
static const u8 effect_tool_sdk_name[16] 			= "AC695X";
static const u8 effect_tool_ver[4] 			    	= {0, 4, 3, 0};
static const u8 effect_tool_password[16] 			= "000";

typedef struct {
    int cmd;     			///<EQ_ONLINE_CMD
    /* int modeId; */
    int data[45];       	///<data
}  EFFECTS_ONLINE_PACKET;

typedef struct password {
    int len;
    char pass[45];
} PASSWORD;

struct __effect_tool {
    u32 eq_type : 3;
    spinlock_t lock;
    uint8_t password_ok;
    struct __effect_mode_cfg 	*parm;
    struct __tool_callback      *cb;
};

static struct __effect_tool *p_tool = NULL;
#define __this  p_tool

static int effect_tool_get_version(EFFECTS_ONLINE_PACKET *packet)
{
    struct __ver_info {
        char sdkname[16];
        u8 eqver[4];
    };
    struct __ver_info effects_ver_info = {0};
    memcpy(effects_ver_info.sdkname, effect_tool_sdk_name, sizeof(effect_tool_sdk_name));
    memcpy(effects_ver_info.eqver, effect_tool_ver, sizeof(effect_tool_ver));
    ci_send_packet(EFFECTS_CONFIG_ID, (u8 *)&effects_ver_info, sizeof(struct __ver_info));
    printf("effect_tool_get_version\n");
    return 0;
}
static int  effect_tool_get_passwork(EFFECTS_ONLINE_PACKET *packet)
{
    uint8_t password = 1;
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&password, sizeof(uint8_t));
    return 0;
}
static int  effect_tool_get_verify_passwork(EFFECTS_ONLINE_PACKET *packet)
{
    //check password
    int len = 0;
    char pass[64];
    PASSWORD ps = {0};
    spin_lock(&__this->lock);
    memcpy(&ps, packet->data, sizeof(PASSWORD));
    /* printf("ps.len %d\n",ps.len); */
    memcpy(&ps, packet->data, sizeof(int) + ps.len);
    /* put_buf(ps.pass, ps.len); */
    spin_unlock(&__this->lock);

    //strcmp xxx
    uint8_t password_ok = 0;
    if (!strcmp(ps.pass, (const char *)effect_tool_password)) {
        password_ok = 1;
    } else {
        log_error("password verify fail \n");
    }
    __this->password_ok = password_ok;
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&password_ok, sizeof(uint8_t));
    return 0;
}
static int effect_tool_get_cfg_filesize(EFFECTS_ONLINE_PACKET *packet)
{
    if (!__this->password_ok) {
        log_error("pass not verify\n");
        return -EINVAL;
    }
    struct __file_size {
        int id;
        int fileid;
    };
    struct __file_size fs;
    memcpy(&fs, packet, sizeof(struct __file_size));
    if (fs.fileid != 0x2) {
        log_error("fs.fileid %d\n", fs.fileid);
        return -EINVAL;
    }

    FILE *file = NULL;
    file = fopen(__this->parm->file_path, "r");
    u32 file_size  = 0;
    if (!file) {
        log_error("effect_cfg.bin err %s\n", __this->parm->file_path);
        /* return -EINVAL; */
    } else {
        file_size = flen(file);
        fclose(file);
    }
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&file_size, sizeof(u32));
    return 0;
}
static int effect_tool_get_cfg_filedata(EFFECTS_ONLINE_PACKET *packet)
{
    if (!__this->password_ok) {
        log_error("pass not verify\n");
        return -EINVAL;
    }

    struct __file_d {
        int id;
        int fileid;
        int offset;
        int size;
    };
    struct __file_d fs;
    memcpy(&fs, packet, sizeof(struct __file_d));
    if (fs.fileid != 0x2) {
        log_error("fs.fileid %d\n", fs.fileid);
        return -EINVAL;
    }
    FILE *file = NULL;
    file = fopen(__this->parm->file_path, "r");
    if (!file) {
        return -EINVAL;
    }
    fseek(file, fs.offset, SEEK_SET);
    u8 *data = malloc(fs.size);
    if (!data) {
        fclose(file);
        return -EINVAL;
    }
    int ret = fread(file, data, fs.size);
    if (ret != fs.size) {
    }
    fclose(file);
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)data, fs.size);
    free(data);
    return 0;
}
static int effect_tool_get_eq_section_num(EFFECTS_ONLINE_PACKET *packet)
{
    uint8_t hw_section = EFFECT_EQ_SECTION_MAX;
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&hw_section, sizeof(uint8_t));
    log_i("id %x, hw_section %d\n", id, hw_section);
    return 0;
}
static int effect_tool_get_mode_counter(EFFECTS_ONLINE_PACKET *packet)
{
    //模式个数
    int mode_cnt = __this->parm->mode_max;
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&mode_cnt, sizeof(int));
    return 0;
}
static int effect_tool_get_mode_name(EFFECTS_ONLINE_PACKET *packet)
{
    //utf8编码得名字
    struct __cmd_name {
        int id;
        int modeId;
    };
    struct __cmd_name cmd;
    memcpy(&cmd, packet, sizeof(struct __cmd_name));

    /* printf("cmd.modeId %d\n", cmd.modeId); */
    /* printf("sizeof(name[modeId])+ 1 %d\n", sizeof(name[cmd.modeId])); */
    u8 tmp[16] = {0};
    if (cmd.modeId >= __this->parm->mode_max) {
        return -EINVAL;
    }
    if (strlen(__this->parm->attr[cmd.modeId].name) >= 16) {
        log_info("effect mode name size overlimit = %d\n", __this->parm->attr[cmd.modeId].name);
        return -EINVAL;
    }
    memcpy(tmp, __this->parm->attr[cmd.modeId].name, strlen(__this->parm->attr[cmd.modeId].name));
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)tmp, 16);
    /* log_info("name[modeId] %s\n", __this->parm->attr[cmd.modeId].name); */
    /* put_buf(__this->parm->attr[cmd.modeId].name, 16); */
    return 0;
}
static int effect_tool_get_mode_cmd_tab_counter(EFFECTS_ONLINE_PACKET *packet)
{
    struct __cmd_tab_cnt {
        int id;
        int modeId;
    };
    struct __cmd_tab_cnt cmd;
    memcpy(&cmd, packet, sizeof(struct __cmd_tab_cnt));
    if (cmd.modeId >= __this->parm->mode_max) {
        return -EINVAL;
    }
    u32 id = packet->cmd;
    int attr_num = __this->parm->attr[cmd.modeId].num;
    ci_send_packet(id, (u8 *)&attr_num, sizeof(int));
    /* log_info("attr_num = %d\n", attr_num); */
    return 0;
}
static int effect_tool_get_mode_cmd_tab(EFFECTS_ONLINE_PACKET *packet)
{
    struct __cmd_tab {
        int id;
        int modeId;
        int offset;
        int count;
    };
    struct __cmd_tab cmd;
    memcpy(&cmd, packet, sizeof(struct __cmd_tab));
    if (cmd.modeId >= __this->parm->mode_max) {
        return -EINVAL;
    }
    u16 *group_tmp = __this->parm->attr[cmd.modeId].cmd_tab;
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&group_tmp[cmd.offset], cmd.count * sizeof(u16));
    /* log_info("mode cmd = \n"); */
    /* put_buf(&group_tmp[cmd.offset], cmd.count*sizeof(u16)); */
    return 0;
}
static int effect_tool_get_mode_eq_group_count(EFFECTS_ONLINE_PACKET *packet)
{
    u32 eq_group_num = 1;
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&eq_group_num, sizeof(u32));
    return 0;
}
static int effect_tool_get_mode_eq_group_range(EFFECTS_ONLINE_PACKET *packet)
{
    u16 groups_cnt[] = {0x1009};
    struct __cmd_eq_range {
        int id;
        int offset;
        int count;
    };
    struct __cmd_eq_range cmd;
    memcpy(&cmd, packet, sizeof(struct __cmd_eq_range));

#ifdef EFFECTS_DEBUG
    log_info("eq group cmd.offset %d, cmd.count %d\n", cmd.offset, cmd.count);
#endif
    u16 g_id[32];
    memcpy(g_id, &groups_cnt[cmd.offset], cmd.count * sizeof(u16));
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&g_id[cmd.offset], cmd.count * sizeof(u16));

    return 0;
}
static int effect_tool_change_mode(EFFECTS_ONLINE_PACKET *packet)
{
    struct __cmd_change_mode {
        int id;
        int modeId;
    };
    struct __cmd_change_mode cmd;
    memcpy(&cmd, packet, sizeof(struct __cmd_change_mode));
    if (cmd.modeId >= __this->parm->mode_max) {
        log_e("mode err = %d\n", cmd.modeId);
        return -EINVAL;
    }

    if (__this->cb->change_mode) {
        __this->cb->change_mode(cmd.modeId);
    }

    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)"OK", 2);
    return 0;
}
static int effect_tool_mode_seq_number(EFFECTS_ONLINE_PACKET *packet)
{
    struct __cmd_seq_number {
        int id;
        int modeId;
    };
    struct __cmd_seq_number cmd;
    memcpy(&cmd, packet, sizeof(struct __cmd_seq_number));
    if (cmd.modeId >= __this->parm->mode_max) {
        return -EINVAL;
    }
    u32 seq_number = MODE_INDEX_TO_SEQ(cmd.modeId);
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)&seq_number, sizeof(u32));
    /* log_info("id = %d, seq = %x\n", cmd.modeId, seq_number); */
    return 0;
}
static int effect_tool_cmd_inquire(EFFECTS_ONLINE_PACKET *packet)
{
    u32 id = packet->cmd;
    ci_send_packet(id, (u8 *)"NO", 2);//OK表示需要重传，NO表示不需要重传,ER还是表示未知命令
    return 0;
}

static int effect_tool_normal_cmd(EFFECTS_ONLINE_PACKET *packet)
{
    if (__this == NULL) {
        return -EFAULT;
    }
    int res = -EINVAL;
    switch (packet->cmd) {
    case EFFECTS_ONLINE_CMD_GETVER:
        res = effect_tool_get_version(packet);
        break;
    case EFFECTS_ONLINE_CMD_PASSWORD:
        res = effect_tool_get_passwork(packet);
        break;
    case EFFECTS_ONLINE_CMD_VERIFY_PASSWORD:
        res = effect_tool_get_verify_passwork(packet);
        break;
    case EFFECTS_ONLINE_CMD_FILE_SIZE:
        res = effect_tool_get_cfg_filesize(packet);
        break;
    case EFFECTS_ONLINE_CMD_FILE:
        res = effect_tool_get_cfg_filedata(packet);
        break;
    case EFFECTS_EQ_ONLINE_CMD_GET_SECTION_NUM:
        res = effect_tool_get_eq_section_num(packet);
        break;
    case EFFECTS_ONLINE_CMD_MODE_COUNT:
        res = effect_tool_get_mode_counter(packet);
        break;
    case EFFECTS_ONLINE_CMD_MODE_NAME:
        res = effect_tool_get_mode_name(packet);
        break;
    case EFFECTS_ONLINE_CMD_MODE_GROUP_COUNT:
        res = effect_tool_get_mode_cmd_tab_counter(packet);
        break;
    case EFFECTS_ONLINE_CMD_MODE_GROUP_RANGE:
        res = effect_tool_get_mode_cmd_tab(packet);
        break;
    case EFFECTS_ONLINE_CMD_EQ_GROUP_COUNT:
        res = effect_tool_get_mode_eq_group_count(packet);
        break;
    case EFFECTS_ONLINE_CMD_EQ_GROUP_RANGE:
        res = effect_tool_get_mode_eq_group_range(packet);
        break;
    case EFFECTS_EQ_ONLINE_CMD_CHANGE_MODE:
        res = effect_tool_change_mode(packet);
        break;
    case EFFECTS_ONLINE_CMD_MODE_SEQ_NUMBER:
        res = effect_tool_mode_seq_number(packet);
        break;
    case EFFECTS_ONLINE_CMD_INQUIRE://pc 查询小鸡是否在线
        res = effect_tool_cmd_inquire(packet);
        break;
    default:
        break;
    }
    return res;
}

static int effect_tool_update(EFFECTS_ONLINE_PACKET *packet, uint16_t size)
{
    log_info("effects_cmd:0x%x ", packet->cmd);
    if (__this == NULL) {
        return -EFAULT;
    }
    if (!__this->password_ok) {
        return -EPERM;
    }
    int res = -EINVAL;
    if (__this->cb && __this->cb->cmd_func) {
        res = __this->cb->cmd_func(__this->cb->priv, packet->data[0], packet->cmd, (u8 *)&packet->data[1], size - sizeof(int) - 1, 1);
    }

    return res;
}

static void effect_tool_callback(uint8_t *packet, uint16_t size)
{
    int res;
    if (__this == NULL) {
        return ;
    }
    ASSERT(((int)packet & 0x3) == 0, "buf %x size %d\n", (unsigned int)packet, size);

    res = effect_tool_update((EFFECTS_ONLINE_PACKET *)packet, size);
    if (res == 0) {
        log_info("Ack");
        ci_send_packet(EFFECTS_CONFIG_ID, (u8 *)"OK", 2);
    } else {
        res = effect_tool_normal_cmd((EFFECTS_ONLINE_PACKET *)packet);
        if (res == 0) {
            return ;
        }
        log_info("Nack");
        ci_send_packet(EFFECTS_CONFIG_ID, (u8 *)"ER", 2);
    }
}

int effect_tool_open(struct __effect_mode_cfg *parm, struct __tool_callback *cb)
{
    if (parm == NULL || cb == NULL) {
        return  -EINVAL;
    }

    struct __effect_tool *tool = zalloc(sizeof(struct __effect_tool));
    if (tool == NULL) {
        return  -EINVAL;
    }

    spin_lock_init(&tool->lock);
    tool->cb = cb;
    tool->parm = parm;

    __this = tool;
    __this->eq_type = EFFECTS_TYPE_ONLINE;
    printf("effect_tool_open ok\n");

    return 0;
}

void effect_tool_close(void)
{
    if (__this) {
        free(__this);
        __this = NULL;
    }
}

REGISTER_CONFIG_TARGET(effect_tool_target) = {
    .id         = EFFECTS_CONFIG_ID,
    .callback   = effect_tool_callback,
};

//在线调试不进power down
static u8 effect_tool_idle_query(void)
{
    if (!__this) {
        return 1;
    }
    return 0;
}

REGISTER_LP_TARGET(effect_tool_lp_target) = {
    .name = "effect_tool",
    .is_idle = effect_tool_idle_query,
};
#endif//TCFG_MIC_EFFECT_ONLINE_ENABLE

#endif//TCFG_MIC_EFFECT_ENABLE
