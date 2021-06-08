#include "file_transfer.h"
#include "system/includes.h"
#include "system/fs/fs.h"
#include "btstack_3th_protocol_user.h"
#include "dev_manager.h"

#if (SMART_BOX_EN && TCFG_DEV_MANAGER_ENABLE)

#define FTP_FILE_DATA_UNIT						(512)     //(234)//不能超过RCSP协议的MTU大小
#define FTP_FILE_DATA_RECIEVE_REMAIN_SIZE		(128) 	  //接收缓存预留
#define FTP_FILE_DATA_RECIEVE_BUF_MIN_SIZE		(FTP_FILE_DATA_UNIT + FTP_FILE_DATA_RECIEVE_REMAIN_SIZE)
#define FTP_FILE_DATA_RECIEVE_TIMEOUT			(3*1000)  //超时拉取数据等待时长， 超时时间到了，自动拉取
#define FTP_DOWNLOAD_FOLDER_NAME				"downlad" //下载目录
#define FTP_FILE_VAILD_MARK						"JL_FTP"  //断点续传需要用到的文件内容标记
#define FTP_FILE_VAILD_MARK_TIMER_UNIT			(10*1000) //单位ms
#define FTP_FILE_CRC_CAC_MAX_COUNTER			(100)	  //文件校验一次校验的数据块个数

//#define FTP_DEBUG_ENABLE
#ifdef FTP_DEBUG_ENABLE
#define ftp_printf	printf
#else
#define ftp_printf(...)
#endif//FTP_DEBUG_ENABLE

enum {
    FTP_END_REASON_NONE = 0,			//没有错误
    FTP_END_REASON_WRITE_ERR,			//写失败
    FTP_END_REASON_DATA_OVER_LIMIT, 	//数据超范围
    FTP_END_REASON_DATA_CRC_ERR,    	//文件校验失败
};

struct __file_check {
    u16  crc_tmp;
    u32  counter;
    u32  remain;
};


struct __ftp_download {
    u32  file_size;
    u32  file_offset;
    u8	 win[FTP_FILE_DATA_UNIT];
    u8   packet_id;
    u8   packet_id_max;
    u8	 last_packet;
    u16  mark_timer;
    u16  get_timeout;
    u16  start_timerout;
    u16  file_crc;
    struct __file_check check;
    char *filepath;
    FILE *file;
    struct __dev *dev;
    void (*end_callback)(void);
};
static struct __ftp_download *ftp_d = NULL;

static void file_transfer_download_get_data(void);
static void __file_transfer_download_file_check_caculate(void *priv);

//*----------------------------------------------------------------------------*/
/**@brief    校验分次处理消息发送处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_file_check_continue(void)
{
    int msg[4];
    msg[0] = (int)__file_transfer_download_file_check_caculate;
    msg[1] = 1;
    msg[2] = 0;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

//*----------------------------------------------------------------------------*/
/**@brief    文件数据块crc校验
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void __file_transfer_download_file_check_caculate(void *priv)
{
    if (ftp_d == NULL) {
        return ;
    }
    wdt_clear();
    JL_ERR err = 0;
    u32 cnt;
    if (ftp_d->check.counter >= FTP_FILE_CRC_CAC_MAX_COUNTER)		{
        cnt = FTP_FILE_CRC_CAC_MAX_COUNTER;
    } else {
        cnt = ftp_d->check.counter;
    }
    printf("cnt = %d, check.counter = %d, crc_tmp = %x\n", cnt, ftp_d->check.counter, ftp_d->check.crc_tmp);
    for (int i = 0; i < cnt; i++) {
        fread(ftp_d->file, ftp_d->win, sizeof(ftp_d->win));
        ftp_d->check.crc_tmp = CRC16_with_initval(ftp_d->win, sizeof(ftp_d->win), ftp_d->check.crc_tmp);
    }
    ftp_d->check.counter -= cnt;
    if (ftp_d->check.counter == 0) {
        if (ftp_d->check.remain) {
            fread(ftp_d->file, ftp_d->win, ftp_d->check.remain);
            printf("remain\n");
            put_buf(ftp_d->win, ftp_d->check.remain);
            ftp_d->check.crc_tmp = CRC16_with_initval(ftp_d->win, ftp_d->check.remain, ftp_d->check.crc_tmp);
        }
        //crc_check end
        if (ftp_d->check.crc_tmp == ftp_d->file_crc) {
            printf("crc check ok!!, start rename\n");
            fseek(ftp_d->file, ftp_d->file_size, SEEK_SET);
            //进入重命名流程
            err = JL_CMD_send(JL_OPCODE_FILE_RENAME, NULL, 0, 1);
            if (err) {
                //命令发送失败， 直接停止
                file_transfer_close();
            }
            return ;
        } else {
            printf("crc check fail, crc_tmp = %x, file_crc = %x!!\n", ftp_d->check.crc_tmp, ftp_d->file_crc);
            u8 reason = FTP_END_REASON_DATA_CRC_ERR;
            fdelete(ftp_d->file);
            ftp_d->file = NULL;
            JL_ERR err = JL_CMD_send(JL_OPCODE_FILE_TRANSFER_END, &reason, 1, 1);
            if (err) {
                //命令发送失败， 直接停止
                file_transfer_close();
            }
        }
    } else {
        file_transfer_download_file_check_continue();
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件文件校验处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_file_check(void)
{
    ftp_d->check.crc_tmp = 0;
    ftp_d->check.counter = ftp_d->file_size / sizeof(ftp_d->win);
    ftp_d->check.remain = ftp_d->file_size % sizeof(ftp_d->win);
    ftp_d->file_offset = ftp_d->file_size;
    printf("crc_tmp = %x, counter = %d, remain = %d\n", ftp_d->check.crc_tmp, ftp_d->check.counter, ftp_d->check.remain);
    fseek(ftp_d->file, 0, SEEK_SET);
    file_transfer_download_file_check_continue();
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输重命名
   @param    rename:重命名
   @return
   @note	 此过程接收的数据是实际文件传输的内容
*/
/*----------------------------------------------------------------------------*/
static int file_rename(const char *rename)
{
    if (ftp_d && ftp_d->file) {
        //rename之前先关闭文件
        fclose(ftp_d->file);
        ftp_d->file = NULL;
        //重新打开文件
        //printf("ftp_d->filepath:%s, rename:%s\n", ftp_d->filepath, rename);
        ftp_d->file = fopen(ftp_d->filepath, "r");
        //free(path);
        if (ftp_d->file) {
            //文件打开成功， 重命名
            int ret = frename(ftp_d->file, rename);
            if (ret) {
                printf("rename fail\n");
            } else {
                printf("rename ok\n");
            }
            return ret;
        } else {
            printf("rename file open err!!");
        }
    }
    return -1;
}

//*----------------------------------------------------------------------------*/
/**@brief    文件重传文件头信息保存处理
   @param    file:文件句柄,win:临时缓存，offset:文件偏移
   @return
   @note	 文件传输过程， 定时会触发断点续传信息保存处理
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_vaild_mark_fill(FILE *file, u8 *win, u32 offset)
{
    if (file) {
        fseek(file, 0, SEEK_SET);
        memset(win, 0, FTP_FILE_DATA_UNIT);
        memcpy(win, FTP_FILE_VAILD_MARK, strlen(FTP_FILE_VAILD_MARK));
        memcpy(win + strlen(FTP_FILE_VAILD_MARK), &offset, sizeof(offset));
        fwrite(file, win, FTP_FILE_DATA_UNIT);
        fseek(file, offset, SEEK_SET);
        printf("============================================%s, offset = %d\n", __FUNCTION__, offset);
    }
}


//*----------------------------------------------------------------------------*/
/**@brief    断点续传信息解析处理
   @param    file:文件句柄, 文件偏移获取指针
   @return
   @note	 在文件传输开始的时候进行解析， 目的是找到续传文件偏移
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_intermittent_parse(FILE *file, u32 *offset)
{
    if (file) {
        char mark[10] = {0};
        fseek(file, 0, SEEK_SET);
        int rlen = fread(file, mark, strlen(FTP_FILE_VAILD_MARK));
        if (rlen) {
            if (strcmp(mark, FTP_FILE_VAILD_MARK) == 0) {
                printf("find VAILD_MARK !!\n");
                rlen = fread(file, offset, sizeof(u32));
                if (rlen) {
                    printf("read offset ok, %d!!\n", *offset);
                    fseek(file, *offset, SEEK_SET);
                } else {
                    printf("read offset fail!!\n");
                    *offset = 0;
                }
            }
        }
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件下载数据拉取超时处理
   @param    priv:私有参数，
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_get_timeout(void *priv)
{
    ftp_printf("timeout!!!\n");
    //在超时时间内没有收完所需要的数据， 重新拉取数据
    file_transfer_download_get_data();
}

//*----------------------------------------------------------------------------*/
/**@brief    断点续传定时保存处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_vaild_mark_scan(void *priv)
{
    if (ftp_d && ftp_d->file && ftp_d->mark_timer && ftp_d->last_packet == 0) {
        file_transfer_download_vaild_mark_fill(ftp_d->file, ftp_d->win, ftp_d->file_offset);
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件下载数据拉取处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_get_data(void)
{
    if (ftp_d == NULL || ftp_d->file == NULL) {
        ftp_printf("file not ready %d!!\n", __LINE__);
        return;
    }

    u32 file_offset = 0;
    u16 recieve_max = rcsp_packet_write_alloc_len();
    if (recieve_max < FTP_FILE_DATA_RECIEVE_BUF_MIN_SIZE) {
        ftp_printf("not enough buf to recieve!!\n");
        return;
    } else {
        ///预留些接收缓存给其他通信使用
        recieve_max -= FTP_FILE_DATA_RECIEVE_REMAIN_SIZE;
    }

    ftp_d->packet_id = 0;
    ftp_d->packet_id_max = recieve_max / FTP_FILE_DATA_UNIT;
    recieve_max = ftp_d->packet_id_max * FTP_FILE_DATA_UNIT;

    if (ftp_d->last_packet) {
        recieve_max = FTP_FILE_DATA_UNIT;
        file_offset = 0;
    } else {
        file_offset = ftp_d->file_offset;
    }
    ftp_printf("[get]recieve_max:%d, file_offset %d\n", recieve_max, file_offset);
    u8 parm[7] = {0};
    parm[0] = 0;
    WRITE_BIG_U16(parm + 1, recieve_max);
    WRITE_BIG_U32(parm + 3, file_offset);
    JL_ERR err = JL_CMD_send(JL_OPCODE_FILE_TRANSFER, parm, sizeof(parm), 0);
    if (err) {
        ftp_printf("%s fail!!! %d\n", __FUNCTION__, err);
        return;
    }
    if (ftp_d->get_timeout == 0) {
        ftp_d->get_timeout = sys_timeout_add(NULL, file_transfer_download_get_timeout, FTP_FILE_DATA_RECIEVE_TIMEOUT);
    }
    return;
}

//*----------------------------------------------------------------------------*/
/**@brief    文件路径长度统计
   @param    root_path:根目录, folder:文件夹，filename:文件名
   @return   文件路径长度
   @note
*/
/*----------------------------------------------------------------------------*/
static u32 creat_path_len(char *root_path, const char *folder, u8 *name, u16 name_len)
{
    return (strlen(root_path) + strlen(folder) + name_len + strlen("/") + 1);
}

//*----------------------------------------------------------------------------*/
/**@brief    文件路径组装处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void creat_file_path(char *path, char *root_path, const char *folder, u8 *name, u16 name_len)
{
    strcat(path, root_path);
    strcat(path, folder);
    strcat(path, "/");
    //strcat(path, name);
    memcpy(path + strlen(root_path) + strlen(folder) + 1, name, name_len);
    //printf("path = %s\n", path);
}


//*----------------------------------------------------------------------------*/
/**@brief    文件下载开始
   @param    OpCode_SN:数据包序列号，data:数据， len:数据长度
   @return   文件路径长度
   @note
*/
/*----------------------------------------------------------------------------*/
void file_transfer_download_start(u8 OpCode_SN, u8 *data, u16 len)
{
    if (ftp_d) {
        //收到了文件传输开始命令， 停止启动超时处理
        if (ftp_d->start_timerout) {
            sys_timeout_del(ftp_d->start_timerout);
            ftp_d->start_timerout = 0;
        }
    }

    JL_ERR err = 0;
    ///创建文件
    u32 file_size = READ_BIG_U32(data);//占用4byte
    u16 file_crc = READ_BIG_U16(data + 4);//占用2byte
    u8 *file_name = data + 4 + 2;
    u16 file_name_len = len - 6;
    if (ftp_d == NULL) {
        ftp_d = zalloc(sizeof(struct __ftp_download));
        if (ftp_d) {
            printf("%s, no mem\n", __FUNCTION__);
            JL_CMD_response_send(JL_OPCODE_FILE_TRANSFER_START, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
            return ;
        }
    }

    ftp_printf("file_size = %d, file_crc = %x\n", file_size, file_crc);

    struct __dev *dev = dev_manager_find_active(0);
    if (!dev) {
        file_transfer_close();
        ftp_printf("no dev online !!\n");
        JL_CMD_response_send(JL_OPCODE_FILE_TRANSFER_START, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
        return ;
    }
    char *root_path = dev_manager_get_root_path(dev);
    char *folder = FTP_DOWNLOAD_FOLDER_NAME;
    char *path = zalloc(creat_path_len(root_path, folder, file_name, file_name_len));

    u8 new_file = 0;
    ASSERT(path);
    creat_file_path(path, root_path, folder, file_name, file_name_len);
    ftp_d->file = fopen(path, "r");
    if (ftp_d->file) {
        printf("file exist\n");
        new_file = 0;
        fclose(ftp_d->file);
        ftp_d->file = NULL;
    } else {
        printf("file new\n");
        new_file = 1;
    }

    ftp_d->file = fopen(path, "w+");
    ftp_d->filepath = path;
    //free(path);
    if (ftp_d->file == NULL) {
        ftp_printf("file create err\n");
        file_transfer_close();
        ///文件打开失败， 回复APP文件传输失败
        JL_CMD_response_send(JL_OPCODE_FILE_TRANSFER_START, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
        return ;
    }
    ftp_d->dev = dev;
    ftp_d->file_offset = 0;
    ftp_d->file_crc = file_crc;
    ftp_d->last_packet = 0;
    ftp_d->file_size = file_size;
    if (ftp_d->file_size > sizeof(ftp_d->win)) {
        if (new_file == 0) {
            ///解析断点续传的文件偏移
            file_transfer_download_intermittent_parse(ftp_d->file, &ftp_d->file_offset);
        }
        if (ftp_d->file_offset == 0 || ftp_d->file_offset > file_size) {
            printf("file offset err !! reset offset \n");
            ftp_d->file_offset = sizeof(ftp_d->win);
            file_transfer_download_vaild_mark_fill(ftp_d->file, ftp_d->win, ftp_d->file_offset);
        } else if (ftp_d->file_offset == file_size) {
            //上次文件已经收完, 进入校验流程
            printf("file aready download end \n");
            file_transfer_download_file_check();
            return ;
        }
    } else {
        ftp_d->last_packet = 1;
    }
    ///回复APP文件传输准备就绪
    u16 file_data_unit = 0;
    WRITE_BIG_U16(&file_data_unit, FTP_FILE_DATA_UNIT);
    err = JL_CMD_response_send(JL_OPCODE_FILE_TRANSFER_START, JL_PRO_STATUS_SUCCESS, OpCode_SN, (u8 *)&file_data_unit, sizeof(u16));
    if (err == 0) {
        printf("%s ok!! file_offset: %d, data unit = %d\n", __FUNCTION__, ftp_d->file_offset, FTP_FILE_DATA_UNIT);
        ///回复启动成功之后，启动数据拉取操作(这里是第一次)
        //fseek(ftp_d->file, ftp_d->file_offset, SEEK_SET);//重定位下文件位置
        file_transfer_download_get_data();
        ftp_d->mark_timer = sys_timer_add(NULL, file_transfer_download_vaild_mark_scan, FTP_FILE_VAILD_MARK_TIMER_UNIT);
        return ;
    } else {
        ftp_printf("%s resp fail!!\n", __FUNCTION__);
        file_transfer_close();
        ftp_d = NULL;
    }
    return;
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输， 重命名处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_transfer_file_rename(u8 status, u8 *data, u16 len)
{
    if (ftp_d)	{
        if (status == JL_PRO_STATUS_SUCCESS)	{
            printf("%s !!, %d\n", __FUNCTION__, __LINE__);
            put_buf(data, len);
            //重命名
            if (file_rename((const char *)data) == 0) {
                u8 reason = FTP_END_REASON_NONE;
                int err = JL_CMD_send(JL_OPCODE_FILE_TRANSFER_END, &reason, 1, 1);
                if (err) {
                    file_transfer_close();
                }
                dev_manager_set_valid(ftp_d->dev, 1);
            } else {
                //有重名的， 重新获取新名称, 如:“xxx_n.mp3”,n为数字
                int err = JL_CMD_send(JL_OPCODE_FILE_RENAME, NULL, 0, 1);
                if (err) {
                    file_transfer_close();
                }
            }
        } else {
            ftp_printf("%s fail!! %d\n", __FUNCTION__, status);
            if (ftp_d->file) {
                //重命名失败，删除文件
                printf("renam file fail, delete file\n");
                fdelete(ftp_d->file);
                ftp_d->file = NULL;
            }
            file_transfer_close();
        }
    }
}


//*----------------------------------------------------------------------------*/
/**@brief    文件下载结束(F->A的回复处理)
   @param    status:命令执行状态，data:数据， len:数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_transfer_download_end(u8 status, u8 *data, u16 len)
{
    if (ftp_d)	{
        ftp_printf("%s status %d\n", __FUNCTION__, status);
        file_transfer_close();
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输，最后一包数据接收处理
   @param    data:数据内容 len:数据长度
   @return
   @note	 文件传输流程是最后才写文件第一包数据
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_download_doing_last_packet(u8 *data, u16 len)
{
    u8 reason = FTP_END_REASON_NONE;
    JL_ERR err = 0;
    if (len > FTP_FILE_DATA_UNIT) {
        printf("last packet data len err!!\n");
        reason = FTP_END_REASON_DATA_OVER_LIMIT;
        err = JL_CMD_send(JL_OPCODE_FILE_TRANSFER_END, &reason, 1, 1);
        if (err) {
            //命令发送失败， 直接停止
            file_transfer_close();
        }
        return ;
    }
    if (ftp_d->get_timeout) {
        sys_timeout_del(ftp_d->get_timeout);
        ftp_d->get_timeout = 0;
    }
    printf("get last packet ok\n");
    fseek(ftp_d->file, 0, SEEK_SET);
    int wlen = fwrite(ftp_d->file, data, len);
    if (wlen != len) {
        ftp_printf("%s err !! %d\n", __FUNCTION__, wlen);
        reason = FTP_END_REASON_WRITE_ERR;//文件写异常
        err = JL_CMD_send(JL_OPCODE_FILE_TRANSFER_END, &reason, 1, 1);
        if (err) {
            //命令发送失败， 直接停止
            file_transfer_close();
        }
    } else {
        //文件接收完成，文件校验
        file_transfer_download_file_check();
    }
    //fseek(ftp_d->file, ftp_d->file_offset, SEEK_SET);
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输过程处理(A->F)
   @param    data:数据， len:数据长度
   @return
   @note	 此过程接收的数据是实际文件传输的内容
*/
/*----------------------------------------------------------------------------*/
void file_transfer_download_doing(u8 *data, u16 len)
{
    if (ftp_d && ftp_d->file) {
        ftp_printf("id = %d, len = %d\n", data[0], len - 1);

        if (ftp_d->packet_id != data[0]) {
            ftp_printf("warning !! packet_id err %d, %d\n", ftp_d->packet_id, data[0]);
            return ;
        }
        len -= 1;
        data += 1;
        ftp_d->packet_id ++;
        u8 reason = FTP_END_REASON_NONE;
        int wlen = 0;
        JL_ERR err = 0;

        if (ftp_d->last_packet) {
            file_transfer_download_doing_last_packet(data, len);
            return ;
        }

        if (ftp_d->file_offset >= ftp_d->file_size) {
            printf("err, file data is over limit!!\n");
            reason = FTP_END_REASON_DATA_OVER_LIMIT;//数据超范围
            err = JL_CMD_send(JL_OPCODE_FILE_TRANSFER_END, &reason, 1, 1);
            if (err) {
                //命令发送失败， 直接停止
                file_transfer_close();
            }
            return ;
        }

        wlen = fwrite(ftp_d->file, data, len);
        if (wlen != len) {
            ftp_printf("%s err !! %d\n", __FUNCTION__, wlen);
            reason = FTP_END_REASON_WRITE_ERR;//文件写异常
            err = JL_CMD_send(JL_OPCODE_FILE_TRANSFER_END, &reason, 1, 1);
            if (err) {
                //命令发送失败， 直接停止
                file_transfer_close();
            }
        } else {
            if (len != FTP_FILE_DATA_UNIT) {
                ftp_printf("data is not a normal unit !!\n");
            }
            ftp_d->file_offset += len;
            ftp_printf("ftp_d->file_offset:%d, ftp_d->file_size:%d, len:%d\n", ftp_d->file_offset, ftp_d->file_size, len);
            if (ftp_d->file_offset >= ftp_d->file_size) {
                ftp_printf("file recieve end!! get file first packet\n");
                if (ftp_d->mark_timer) {
                    sys_timer_del(ftp_d->mark_timer);
                    ftp_d->mark_timer = 0;
                }
                ftp_d->last_packet = 1;
                file_transfer_download_get_data();
                return;
            }
            ftp_d->last_packet = 0;
            if (ftp_d->get_timeout) {
                //接收数据成功，重置一下超时
                ftp_printf("reset timeout!!\n");
                sys_timer_modify(ftp_d->get_timeout, FTP_FILE_DATA_RECIEVE_TIMEOUT);
            }
            if (ftp_d->packet_id >= ftp_d->packet_id_max) {
                //该次数据包已经收完，拉取新的数据
                ftp_printf("get more!!\n");
                file_transfer_download_get_data();
            }
        }
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输，被动取消处理
   @param    OpCode_SN:数据包sn码， 回复的时候用的
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_transfer_download_passive_cancel(u8 OpCode_SN, u8 *data, u16 len)
{
    file_transfer_close();
    printf("passive_cancel answer\n");
    JL_CMD_response_send(JL_OPCODE_FILE_TRANSFER_CANCEL, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输，主动取消处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_transfer_download_active_cancel(void)
{
    if (ftp_d) {
        file_transfer_close();
        JL_CMD_send(JL_OPCODE_FILE_TRANSFER_CANCEL, NULL, 0, 1);
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输，主动取消处理回复处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_transfer_download_active_cancel_response(u8 status, u8 *data, u16 len)
{
    if (status == JL_PRO_STATUS_SUCCESS) {
        printf("active_cancel_response ok!!");
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输关闭处理
   @param
   @return
   @note	 主要是做一些资源释放处理
*/
/*----------------------------------------------------------------------------*/
void file_transfer_close(void)
{
    if (ftp_d) {
        if (ftp_d->check.counter) {
            printf("crc caculating !! close err\n");
            return ;
        }
        if (ftp_d->start_timerout) {
            sys_timeout_del(ftp_d->start_timerout);
            ftp_d->start_timerout = 0;
        }
        if (ftp_d->get_timeout) {
            sys_timeout_del(ftp_d->get_timeout);
            ftp_d->get_timeout = 0;
        }
        if (ftp_d->mark_timer) {
            sys_timer_del(ftp_d->mark_timer);
            ftp_d->mark_timer = 0;
        }
        if (ftp_d->file) {
            if (ftp_d->last_packet == 0) {
                //文件没有传输完， 如果不是直接断电， 有机会执行更新断点续传的文件偏移信息
                file_transfer_download_vaild_mark_fill(ftp_d->file, ftp_d->win, ftp_d->file_offset);
            }
            fclose(ftp_d->file);
        }
        if (ftp_d->filepath) {
            free(ftp_d->filepath);
        }
        if (ftp_d->end_callback) {
            ftp_d->end_callback();
        }
        free(ftp_d);
        ftp_d = NULL;
        printf("file_transfer_close!!!!!!\n");
    }
}

//*----------------------------------------------------------------------------*/
/**@brief    文件传输启动超时处理
   @param
   @return
   @note	 响应app预处理,之后在设定时间内没有发文件传输开始命令过来触发超时
*/
/*----------------------------------------------------------------------------*/
static void file_transfer_start_timeout(void)
{
    printf("%s\n", __FUNCTION__);
    file_transfer_close();
}


//*----------------------------------------------------------------------------*/
/**@brief    文件传输初始化处理
   @param	 end_callback:文件传输结束处理回调
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void file_transfer_init(void (*end_callback)(void))
{
    if (ftp_d) {
        ftp_printf("file downloading err %d!!\n", __LINE__);
        return ;
    }
    ftp_d = zalloc(sizeof(struct __ftp_download));
    if (ftp_d == NULL) {
        if (end_callback) {
            end_callback();
        }
        return ;
    }
    ftp_d->end_callback = end_callback;
    //如果在超时时间内都没有发文件传输开始命令， 退出文件传输流程
    ftp_d->start_timerout = sys_timeout_add(NULL, file_transfer_start_timeout, 2000);
}
#else

void file_transfer_init(void (*end_callback)(void))
{
}
void file_transfer_download_start(u8 OpCode_SN, u8 *data, u16 len)
{
}
void file_transfer_download_end(u8 status, u8 *data, u16 len)
{
}
void file_transfer_download_doing(u8 *data, u16 len)
{
}
void file_transfer_download_passive_cancel(u8 OpCode_SN, u8 *data, u16 len)
{
}
void file_transfer_download_active_cancel(void)
{
}
void file_transfer_download_active_cancel_response(u8 status, u8 *data, u16 len)
{
}
void file_transfer_file_rename(u8 status, u8 *data, u16 len)
{
}
void file_transfer_close(void)
{
}

#endif



