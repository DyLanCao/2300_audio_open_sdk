#include "stdio.h"
#include "string.h"
#include "heap_api.h"
#include "cmsis.h"
// #include "cmsis_os.h"
#include "pmu.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "hal_norflash.h"
#include "norflash_api.h"

#if 0
#define NORFLASH_API_TRACE TRACE
#else
#define NORFLASH_API_TRACE(str,...)
#endif

#define API_IS_ALIGN(v,size)  (((v/size)*size) == v)

static NORFLASH_API_INFO norflash_api_info = {false,};
static heap_handle_t norflash_api_mempool = NULL;
static OPERA_INFO_LIST opera_info_list[NORFLASH_API_OPRA_LIST_LEN];
static DATA_LIST      data_list[NORFLASH_API_WRITE_BUFF_LEN];
// static uint8_t check_buffer[4096];

static int suspend_number = 0;

static void* _norflash_api_malloc(uint32_t size)
{
    uint32_t i;

    if(size == sizeof(OPRA_INFO))
    {
        for(i = 0; i < NORFLASH_API_OPRA_LIST_LEN; i++)
        {
            if(opera_info_list[i].is_used == false)
            {
                opera_info_list[i].is_used = true;
                return (void*)&opera_info_list[i].opera_info;
            }
        }
        return NULL;
    }
    else if(size == NORFLASH_API_SECTOR_SIZE)
    {
        for(i = 0; i < NORFLASH_API_WRITE_BUFF_LEN; i++)
        {
            if(data_list[i].is_used == false)
            {
                data_list[i].is_used = true;
                return (void*)data_list[i].buffer;
            }
        }
        return NULL;
    }
    else
    {
        return heap_malloc(norflash_api_mempool,size);
    }
}

static void _norflash_api_free(void *p)
{
    uint32_t i;

    for(i = 0; i < NORFLASH_API_OPRA_LIST_LEN; i++)
    {
        if((uint8_t*)&opera_info_list[i].opera_info == p)
        {
            opera_info_list[i].is_used = false;
            return;
        }
    }

    for(i = 0; i < NORFLASH_API_WRITE_BUFF_LEN; i++)
    {
        if(data_list[i].buffer == p)
        {
            data_list[i].is_used = false;
            return;
        }
    }

    heap_free(norflash_api_mempool,p);
}

static MODULE_INFO* _get_module_info(enum NORFLASH_API_MODULE_ID_T mod_id)
{
    return &norflash_api_info.mod_info[mod_id];
}

static OPRA_INFO* _get_tail(MODULE_INFO *mod_info,bool is_remove)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *pre_node = NULL;
    OPRA_INFO *tmp;

    pre_node = mod_info->opera_info;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;
        if(tmp)
        {
            pre_node = opera_node;
        }
    }
    if(is_remove)
    {
        if(pre_node)
        {
            pre_node->next = NULL;
        }
    }
    if(opera_node)
    {
        opera_node->lock = true;
    }
    return opera_node;
}

static void _opera_del(MODULE_INFO *mod_info,OPRA_INFO *node)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *pre_node = NULL;
    OPRA_INFO *tmp;

    pre_node = mod_info->opera_info;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        if(opera_node == node)
        {
            if(mod_info->opera_info == opera_node)
            {
                mod_info->opera_info = NULL;
            }
            else
            {
                pre_node->next = NULL;
            }
            if(node->buff)
            {
                _norflash_api_free(node->buff);
            }
            _norflash_api_free(node);
            break;
        }
        tmp = opera_node->next;
        if(tmp)
        {
            pre_node = opera_node;
        }
    }
}

static uint32_t _get_ew_count(MODULE_INFO *mod_info)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *tmp;
    uint32_t count = 0;

    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        count ++;
        tmp = opera_node->next;
    }
    return count;
}

static uint32_t _get_w_count(MODULE_INFO *mod_info)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *tmp;
    uint32_t count = 0;

    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        if(opera_node->type == NORFLASH_API_WRITTING)
        {
            count ++;
        }
        tmp = opera_node->next;
    }
    return count;
}

static uint32_t _get_e_count(MODULE_INFO *mod_info)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *tmp;
    uint32_t count = 0;

    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        if(opera_node->type == NORFLASH_API_ERASING)
        {
            count ++;
        }
        tmp = opera_node->next;
    }
    return count;
}

static MODULE_INFO* _get_cur_mod(void)
{
    uint32_t i;
    MODULE_INFO *mod_info;
    uint32_t tmp_mod_id = NORFLASH_API_MODULE_ID_COUNT;

    if(norflash_api_info.cur_mod)
    {
        return norflash_api_info.cur_mod;
    }

    tmp_mod_id = norflash_api_info.cur_mod_id;
    for(i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++)
    {
        tmp_mod_id =  tmp_mod_id + 1 >= NORFLASH_API_MODULE_ID_COUNT ? 0 : tmp_mod_id + 1;
        mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)tmp_mod_id);
        if(mod_info->is_inited)
        {
            if(_get_ew_count(mod_info) > 0)
            {
                return mod_info;
            }
        }
    }
    return NULL;
}

static enum NORFLASH_API_MODULE_ID_T _get_mod_id(MODULE_INFO *mod_info)
{
    uint32_t i;
    enum NORFLASH_API_MODULE_ID_T mod_id = NORFLASH_API_MODULE_ID_COUNT;
    MODULE_INFO *tmp_mod_info;

    for(i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++)
    {
        tmp_mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
        if(tmp_mod_info == mod_info)
        {
            mod_id = (enum NORFLASH_API_MODULE_ID_T)i;
            break;
        }
    }
    return mod_id;
}

static int32_t _opera_read(MODULE_INFO *mod_info,
               uint32_t addr,
               uint8_t *buff,
               uint32_t len)
{
    OPRA_INFO *opera_node;
    OPRA_INFO *e_node = NULL;
    OPRA_INFO *w_node = NULL;
    OPRA_INFO *tmp;
    uint32_t r_offs;
    uint32_t sec_start;
    uint32_t sec_len;

    sec_len = mod_info->mod_sector_len;
    sec_start = (addr/sec_len)*sec_len;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;
        if(opera_node->addr == sec_start)
        {
            if(opera_node->type == NORFLASH_API_WRITTING)
            {
                w_node = opera_node;
                break;
            }
            else
            {
                e_node = opera_node;
                break;
            }
        }
    }

    if(w_node)
    {
        r_offs = addr - sec_start;
        memcpy(buff,w_node->buff + r_offs,len);
    }
    else
    {
        if(e_node)
        {
            memset(buff,0xff,len);
        }
        else
        {
            memcpy(buff,(uint8_t*)addr,len);
            /*
            HAL_NORFLASH_RET_T result;
            result = hal_norflash_read(mod_info->dev_id,addr,buff,len);
            if(result != HAL_NORFLASH_OK)
            {
                NORFLASH_API_TRACE("%s: hal_norflash_read failed,result = %d.",
                        __func__,result);
                return result;
            }
            */
        }
    }
    return 0;
}


static int32_t _opera_cmp(MODULE_INFO *mod_info,
               uint32_t addr,
               uint8_t *buff,
               uint32_t len)
{
    OPRA_INFO *opera_node;
    OPRA_INFO *e_node = NULL;
    OPRA_INFO *w_node = NULL;
    OPRA_INFO *tmp;
    uint32_t r_offs;
    uint32_t sec_start;
    uint32_t sec_len;
    uint32_t i;
    int32_t ret = 0;

    sec_len = mod_info->mod_sector_len;
    sec_start = (addr/sec_len)*sec_len;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;
        if(opera_node->addr == sec_start)
        {
            if(opera_node->type == NORFLASH_API_WRITTING)
            {
                w_node = opera_node;
                break;
            }
            else
            {
                e_node = opera_node;
                break;
            }
        }
    }

    if(w_node)
    {
        r_offs = addr - sec_start;
        ret = memcmp(buff,w_node->buff + r_offs,len);
    }
    else
    {
        if(e_node)
        {
            for(i = 0; i < len;i++)
            {
                if(buff[i] != 0xff)
                {
                    ret = -1;
                    break;
                }
            }
        }
        else
        {
            ret = memcmp(buff,(uint8_t*)addr,len);
            /*
            r_buff = (uint8_t*)_norflash_api_malloc(sec_len);
            if(!r_buff)
            {
                ret = 1;
            }
            else
            {
                uint8_t *r_buff;
                HAL_NORFLASH_RET_T result;
                result = hal_norflash_read(mod_info->dev_id,sec_start,r_buff,sec_len);
                if(result != HAL_NORFLASH_OK)
                {
                    NORFLASH_API_TRACE("%s: hal_norflash_read failed,result = %d.",
                            __func__,result);
                    _norflash_api_free(r_buff);
                    return result;
                }
                r_offs = addr - sec_start;
                ret = memcmp(r_buff + r_offs,buff,len);
                _norflash_api_free(r_buff);
            }
            */
        }
    }
    return ret;
}


static int32_t _e_opera_add(MODULE_INFO *mod_info,
               uint32_t addr,
               uint32_t len
               )
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *pre_node = NULL;
    OPRA_INFO *tmp;
    int32_t ret = 0;

    // delete opera nodes with the same address when add the erase opera node.
    pre_node = mod_info->opera_info;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;

        if(opera_node->addr == addr)
        {
            if(opera_node->lock == false)
            {
                if(opera_node == mod_info->opera_info)
                {
                    mod_info->opera_info = tmp;
                }
                else
                {
                    pre_node->next = tmp;
                }
                if(opera_node->type == NORFLASH_API_WRITTING)
                {
                    if(opera_node->buff)
                    {
                        _norflash_api_free(opera_node->buff);
                    }
                }
                _norflash_api_free(opera_node);
            }
            else
            {
                if(opera_node->type == NORFLASH_API_ERASING)
                {
                    NORFLASH_API_TRACE("%s: erase is merged! addr = 0x%x,len = 0x%x.",
                        opera_node->addr,
                        opera_node->len);
                    ret = 0;
                    goto _func_end;
                }
            }
        }

        if(tmp)
        {
            pre_node = opera_node;
        }
    }

    // add new node to header.
    opera_node = (OPRA_INFO*)_norflash_api_malloc(sizeof(OPRA_INFO));
    if(opera_node == NULL)
    {
        // ASSERT(opera_node,"%s:%d,_norflash_api_malloc failed! size = %d.",
        //        __func__,__LINE__,sizeof(OPRA_INFO));
        NORFLASH_API_TRACE("%s:%d,_norflash_api_malloc failed! size = %d.",
                __func__,__LINE__,sizeof(OPRA_INFO));
        ret = 1;
        goto _func_end;
    }
    opera_node->type = NORFLASH_API_ERASING;
    opera_node->addr = addr;
    opera_node->len = len;
    opera_node->w_offs = 0;
    opera_node->w_len = 0;
    opera_node->buff = NULL;
    opera_node->lock = false;
    opera_node->next = mod_info->opera_info;
    mod_info->opera_info = opera_node;
    ret = 0;
_func_end:

    return ret;
}

static int32_t _w_opera_add(MODULE_INFO *mod_info,
               uint32_t addr,
               uint32_t len,
               uint8_t *buff)
{
    OPRA_INFO *opera_node = NULL;
    OPRA_INFO *e_node = NULL;
    OPRA_INFO *w_node = NULL;
    OPRA_INFO *tmp;
    uint32_t w_offs;
    uint32_t w_len;
    uint32_t sec_start;
    uint32_t sec_len;
    uint32_t w_end1;
    uint32_t w_end2;
    uint32_t w_start;
    uint32_t w_end;
    uint32_t w_len_new;
    int32_t ret = 0;

    sec_len = mod_info->mod_sector_len;
    sec_start = (addr/sec_len)*sec_len;
    w_offs = addr - sec_start;
    w_len = len;
    tmp = mod_info->opera_info;
    while(tmp)
    {
        opera_node = tmp;
        tmp = opera_node->next;

        if(opera_node->addr == sec_start)
        {
            if(opera_node->type == NORFLASH_API_WRITTING)
            {
                if(!opera_node->lock)
                {
                    // select the first w_node in the list.
                    w_node = opera_node;
                    break;
                }
            }
            else
            {
                e_node = opera_node;
                break;
            }
        }
    }

    if(w_node)
    {
        memcpy(w_node->buff + w_offs,buff,w_len);
        w_start = w_node->w_offs <= w_offs ? w_node->w_offs:w_offs;
        w_end1 = w_node->w_offs + w_node->w_len;
        w_end2 = w_offs + w_len;
        w_end = w_end1 >= w_end2 ? w_end1 : w_end2;
        w_len_new = w_end - w_start;
        w_node->w_offs = w_start;
        w_node->w_len = w_len_new;
        opera_node = w_node;
        ret = 0;
    }
    else
    {
        opera_node = (OPRA_INFO*)_norflash_api_malloc(sizeof(OPRA_INFO));
        if(opera_node == NULL)
        {
            // ASSERT(opera_node,"%s:%d,_norflash_api_malloc failed! size = %d.",
            //        __func__,__LINE__,sizeof(OPRA_INFO));
            NORFLASH_API_TRACE("%s:%d,_norflash_api_malloc failed! size = %d.",
                    __func__,__LINE__,sizeof(OPRA_INFO));
             ret = 1;
             goto _func_end;
        }
        opera_node->type = NORFLASH_API_WRITTING;
        opera_node->addr = sec_start;
        opera_node->len = sec_len;
        opera_node->w_offs = w_offs;
        opera_node->w_len = w_len;
        opera_node->buff = (uint8_t*)_norflash_api_malloc(opera_node->len);
        if(opera_node->buff == NULL)
        {
            _norflash_api_free(opera_node);
            // ASSERT(opera_node,"%s:%d,_norflash_api_malloc failed! size = 4096.",
            //    __func__,__LINE__,opera_node->len);
            NORFLASH_API_TRACE("%s:%d,_norflash_api_malloc failed! size = %d.",
                    __func__,__LINE__,opera_node->len);
             ret = 1;
             goto _func_end;
        }
        if(e_node)
        {
            memset(opera_node->buff,0xff,opera_node->len);
        }
        else
        {
            memcpy(opera_node->buff,(uint8_t*)opera_node->addr,opera_node->len);
            /*
            HAL_NORFLASH_RET_T result;
            result = hal_norflash_read(mod_info->dev_id,opera_node->addr,opera_node->buff,opera_node->len);
            ASSERT(result == HAL_NORFLASH_OK, "%s:%d,hal_norflash_read failed! result = %d",
                   __func__, __LINE__, result);
            */
        }
        memcpy(opera_node->buff + w_offs,buff,w_len);
        opera_node->lock = false;
        opera_node->next = mod_info->opera_info;
        mod_info->opera_info = opera_node;
        ret = 0;
    }

_func_end:
    return ret;
}

bool _opera_flush(MODULE_INFO *mod_info,bool force_nosuspend)
{
    OPRA_INFO *cur_opera_info;
    enum HAL_NORFLASH_RET_T result;
    bool opera_is_completed = false;
    NORFLASH_API_OPERA_RESULT opera_result;
    bool ret = false;
#if defined(FLASH_SUSPEND)
    bool suspend = true;
#else
    bool suspend = false;
#endif

    suspend = force_nosuspend == true ? false: suspend;
    if(!mod_info->cur_opera_info)
    {
        mod_info->cur_opera_info = _get_tail(mod_info, false);
    }

    if(!mod_info->cur_opera_info)
    {
        return false;
    }

    ret = true;
    cur_opera_info = mod_info->cur_opera_info;
    if(cur_opera_info->type == NORFLASH_API_WRITTING)
    {
        if(mod_info->state == NORFLASH_API_STATE_IDLE)
        {
            suspend_number = 0;
            if(cur_opera_info->w_len > 0)
            {
                NORFLASH_API_TRACE("%s: %d,hal_norflash_write_suspend,addr = 0x%x,len = 0x%x,suspend = %d.",
                                __func__,__LINE__,
                                cur_opera_info->addr + cur_opera_info->w_offs,
                                cur_opera_info->w_len,
                                suspend);
                pmu_flash_write_config();
                result = hal_norflash_write_suspend(mod_info->dev_id,
                             cur_opera_info->addr + cur_opera_info->w_offs,
                             cur_opera_info->buff + cur_opera_info->w_offs,
                             cur_opera_info->w_len,
                             suspend);
                pmu_flash_read_config();
            }
            else
            {
                result = HAL_NORFLASH_OK;
            }

            if(result == HAL_NORFLASH_OK)
            {
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_WRITTING_SUSPEND;
            }
            else
            {
                ASSERT(0, "%s: %d, hal_norflash_write_suspend failed,result = %d",__func__,__LINE__,result);
            }
        }
        else if(mod_info->state == NORFLASH_API_STATE_WRITTING_SUSPEND)
        {
            suspend_number ++;
            pmu_flash_write_config();
            result = hal_norflash_write_resume(mod_info->dev_id,
                         suspend);
            pmu_flash_read_config();
            if(result == HAL_NORFLASH_OK)
            {
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_WRITTING_SUSPEND;
            }
            else
            {
                ASSERT(0, "%s: %d, hal_norflash_write_resume failed,result = %d",__func__,__LINE__,result);
            }
        }
        else
        {
            ASSERT(0, "%s: %d, mod_info->state error,state = %d",__func__,__LINE__,mod_info->state);
        }
    }
    else
    {
        if(mod_info->state == NORFLASH_API_STATE_IDLE)
        {
            suspend_number = 0;
            NORFLASH_API_TRACE("%s: %d,hal_norflash_erase_suspend,addr = 0x%x,len = 0x%x,suspend = %d.",
                                __func__,__LINE__,
                                cur_opera_info->addr,
                                cur_opera_info->len,
                                suspend);
            pmu_flash_write_config();
            result = hal_norflash_erase_suspend(mod_info->dev_id,
                         cur_opera_info->addr,
                         cur_opera_info->len,
                         suspend);
            pmu_flash_read_config();
            if(result == HAL_NORFLASH_OK)
            {
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_ERASE_SUSPEND;
            }
            else
            {
                ASSERT(0, "%s: %d, hal_norflash_erase_suspend failed,result = %d",__func__,__LINE__,result);
            }
        }
        else if(mod_info->state == NORFLASH_API_STATE_ERASE_SUSPEND)
        {
            suspend_number ++;
            pmu_flash_write_config();
            result = hal_norflash_erase_resume(mod_info->dev_id,
                         suspend);
            pmu_flash_read_config();
            if(result == HAL_NORFLASH_OK)
            {
                opera_is_completed = true;
                goto __opera_is_completed;
            }
            else if(result == HAL_NORFLASH_SUSPENDED)
            {
                mod_info->state = NORFLASH_API_STATE_ERASE_SUSPEND;
            }
            else
            {
                ASSERT(0, "%s: %d, hal_norflash_write_resume failed,result = %d",
                       __func__,__LINE__,result);
            }
        }
        else
        {
            ASSERT(0, "%s: %d, mod_info->state error,state = %d",
                __func__,__LINE__,mod_info->state);
        }
    }

__opera_is_completed:

    if(opera_is_completed)
    {
        mod_info->state = NORFLASH_API_STATE_IDLE;
        if(mod_info->cb_func &&
           ((cur_opera_info->w_len > 0 && cur_opera_info->type == NORFLASH_API_WRITTING)
           || (cur_opera_info->len > 0 && cur_opera_info->type == NORFLASH_API_ERASING)))
        {
            NORFLASH_API_TRACE("%s: w/e done.type = %d,addr = 0x%x,w_len = 0x%x,len = 0x%x,suspend_num = %d.",
                __func__,
                cur_opera_info->type,
                cur_opera_info->addr + cur_opera_info->w_offs,
                cur_opera_info->w_len,
                cur_opera_info->len,
                suspend_number);
            if(cur_opera_info->type == NORFLASH_API_WRITTING)
            {
                opera_result.addr = cur_opera_info->addr + cur_opera_info->w_offs;
                opera_result.len = cur_opera_info->w_len;
            }
            else
            {
                opera_result.addr = cur_opera_info->addr;
                opera_result.len = cur_opera_info->len;
            }
            opera_result.type = cur_opera_info->type;
            opera_result.result = NORFLASH_API_OK;
            opera_result.remain_num = _get_ew_count(mod_info) - 1;
            mod_info->cb_func(&opera_result);
        }
        _opera_del(mod_info,cur_opera_info);
        mod_info->cur_opera_info = NULL;
    }

    return ret;
}


//-------------------------------------------------------------------
// APIS Function.
//-------------------------------------------------------------------
enum NORFLASH_API_RET_T norflash_api_init(void)
{
    memset((void*)&norflash_api_info,0,sizeof(NORFLASH_API_INFO));
    norflash_api_info.cur_mod_id = NORFLASH_API_MODULE_ID_COUNT;
    norflash_api_info.is_inited = true;
    norflash_api_info.cur_mod = NULL;
#if !defined(FLASH_API_SIMPLE)
#if defined(FLASH_SUSPEND)
    hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_NORFLASH_API, norflash_api_flush);
#else
    hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_NORFLASH_API, norflash_api_flush);
#endif
#endif
    return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_register(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                enum HAL_NORFLASH_ID_T dev_id,
                uint32_t mod_base_addr,
                uint32_t mod_len,
                uint32_t mod_block_len,
                uint32_t mod_sector_len,
                uint32_t mod_page_len,
                uint32_t buffer_len,
                NORFLASH_API_OPERA_CB cb_func
                )
{
    MODULE_INFO *mod_info;

    NORFLASH_API_TRACE("%s: mod_id = %d,dev_id = %d,mod_base_addr = 0x%x,mod_len = 0x%x,",
            __func__,mod_id,dev_id,mod_base_addr,mod_len);
    NORFLASH_API_TRACE("mod_block_len = 0x%x,mod_sector_len = 0x%x,mod_page_len = 0x%x,buffer_len = 0x%x.",
            mod_block_len,mod_sector_len,mod_page_len,buffer_len);
    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE("%s: %d, norflash_api uninit!",__func__,__LINE__);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE("%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    if(dev_id >= HAL_NORFLASH_ID_NUM)
    {
        NORFLASH_API_TRACE("%s : dev_id error! mod_id = %d.",__func__,dev_id);
        return NORFLASH_API_BAD_DEV_ID;
    }

    if(buffer_len < mod_sector_len || !API_IS_ALIGN(buffer_len,mod_sector_len))
    {
        NORFLASH_API_TRACE("%s : buffer_len error buffer_len = %d.",__func__, buffer_len);
        return NORFLASH_API_BAD_BUFF_LEN;
    }
    mod_info = _get_module_info(mod_id);
    if(mod_info->is_inited)
    {
        // NORFLASH_API_TRACE("%s: %d, norflash_async[%d] has registered!",__func__,__LINE__,mod_id);
        return NORFLASH_API_OK; //NORFLASH_API_ERR_HASINIT;
    }
    mod_info->dev_id = dev_id;
    mod_info->mod_id = mod_id;
    mod_info->mod_base_addr = mod_base_addr;
    mod_info->mod_len = mod_len;
    mod_info->mod_block_len = mod_block_len;
    mod_info->mod_sector_len = mod_sector_len;
    mod_info->mod_page_len = mod_page_len;
    mod_info->buff_len = buffer_len;
    mod_info->cb_func = cb_func;
    mod_info->opera_info = NULL;
    mod_info->cur_opera_info = NULL;
    mod_info->state = NORFLASH_API_STATE_IDLE;
    mod_info->is_inited = true;
    return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_read(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                uint8_t *buffer,
                uint32_t len
                )
{
    MODULE_INFO *mod_info;
    int32_t result;
    enum NORFLASH_API_RET_T ret;
    uint32_t lock;

    NORFLASH_API_TRACE("%s:mod_id = %d,start_addr = 0x%x,len = 0x%x",
                __func__,mod_id, start_addr, len);
    ASSERT(buffer,"%s:buffer is null! ",
                __func__);
    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE("%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE("%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_inited)
    {
        NORFLASH_API_TRACE("%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(start_addr < mod_info->mod_base_addr
       || start_addr + len > mod_info->mod_base_addr + mod_info->mod_len)
    {
        NORFLASH_API_TRACE("%s : reading out of range! start_address = 0x%x,len = 0x%x.",
                    __func__, start_addr, len);
        return NORFLASH_API_BAD_ADDR;
    }

    if(len == 0)
    {
        NORFLASH_API_TRACE("%s : len error! len = %d.",
                   __func__, len);
        return NORFLASH_API_BAD_LEN;
    }
    lock = int_lock_global();
    result = _opera_read(mod_info,start_addr,(uint8_t*)buffer,len);

    if(result)
    {
        ret =  NORFLASH_API_ERR;
    }
    else
    {
        ret = NORFLASH_API_OK;
    }
    int_unlock_global(lock);
    NORFLASH_API_TRACE("%s: done. ret = %d.",__func__,ret);
    return ret;
}

enum NORFLASH_API_RET_T norflash_sync_read(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                uint8_t *buffer,
                uint32_t len
                )
{
    MODULE_INFO *mod_info;

    NORFLASH_API_TRACE("%s:mod_id = %d,start_addr = 0x%x,len = 0x%x",
                __func__,mod_id,start_addr,len);
    ASSERT(buffer,"%s:%d,buffer is null! ",
                __func__,__LINE__);
    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE("%s: norflash_api uninit!", __func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE("%s : mod_id error! mod_id = %d.", __func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_inited)
    {
        NORFLASH_API_TRACE("%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(start_addr < mod_info->mod_base_addr
       || start_addr + len > mod_info->mod_base_addr + mod_info->mod_len)
    {
        NORFLASH_API_TRACE("%s : reading out of range! start_address = 0x%x,len = 0x%x.",
                    __func__, start_addr, len);
        return NORFLASH_API_BAD_ADDR;
    }

    if(len == 0)
    {
        NORFLASH_API_TRACE("%s : len error! len = %d.",
                   __func__, len);
        return NORFLASH_API_BAD_LEN;
    }
    memcpy(buffer,(uint8_t*)start_addr,len);
    NORFLASH_API_TRACE("%s: done.",__func__);
    return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_erase(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                uint32_t len,
                bool async
                )
{
    MODULE_INFO *mod_info;
    MODULE_INFO *cur_mod_info;
    uint32_t lock;
    int32_t result;
    bool bresult;
    enum NORFLASH_API_RET_T ret;

    NORFLASH_API_TRACE("%s: mod_id = %d,start_addr = 0x%x,len = 0x%x,async = %d.",
                __func__,mod_id,start_addr,len,async);
    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE("%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE("%s : invalid mod_id! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_inited)
    {
        NORFLASH_API_TRACE("%s : module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(start_addr < mod_info->mod_base_addr
       || start_addr + len > mod_info->mod_base_addr + mod_info->mod_len)
    {
        NORFLASH_API_TRACE("%s : erase out of range! start_address = 0x%x,len = 0x%x.",
                    __func__,start_addr,len);
        return NORFLASH_API_BAD_ADDR;
    }

    if(!API_IS_ALIGN(start_addr,mod_info->mod_sector_len))
    {
        NORFLASH_API_TRACE("%s : start_address no alignment! start_address = %d.",
                   __func__, start_addr);
        return NORFLASH_API_BAD_ADDR;
    }

    if(len != mod_info->mod_sector_len)
    {
        NORFLASH_API_TRACE("%s : len error. len = %d!",
                   __func__, len);
        return NORFLASH_API_BAD_LEN;
    }
#if defined(FLASH_API_SIMPLE)
    async = false;
#endif
    if(async)
    {
        lock = int_lock_global();

        // add to opera_info chain header.
        result = _e_opera_add(mod_info,start_addr,len);
        if(result == 0)
        {
            ret = NORFLASH_API_OK;
        }
        else
        {
            ret = NORFLASH_API_BUFFER_FULL;
        }
        int_unlock_global(lock);
        NORFLASH_API_TRACE("%s: _e_opera_add done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    else
    {
        lock = int_lock_global();
        // Handle the suspend operation.
        if(norflash_api_info.cur_mod != NULL
           && mod_info != norflash_api_info.cur_mod)
        {
            cur_mod_info = norflash_api_info.cur_mod;
            while(cur_mod_info->state != NORFLASH_API_STATE_IDLE)
            {
                bresult = _opera_flush(cur_mod_info,true);
                if(!bresult)
                {
                    norflash_api_info.cur_mod = NULL;
                }
            }
        }

        // flush all of cur module opera.
        // norflash_api_info.cur_mod_id = mod_id;
        do{
            bresult = _opera_flush(mod_info,true);
        }while(bresult);
        pmu_flash_write_config();
        result = hal_norflash_erase(mod_info->dev_id,start_addr, len);
        pmu_flash_read_config();
        if(result == HAL_NORFLASH_OK)
        {
            ret = NORFLASH_API_OK;
        }
        else if(result == HAL_NORFLASH_BAD_ADDR)
        {
            ret = NORFLASH_API_BAD_ADDR;
        }
        else if(result == HAL_NORFLASH_BAD_LEN)
        {
            ret = NORFLASH_API_BAD_LEN;
        }
        else
        {
            ret = NORFLASH_API_ERR;
        }
        int_unlock_global(lock);
        NORFLASH_API_TRACE("%s: hal_norflash_erase done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    //NORFLASH_API_TRACE("%s: done.ret = %d.",__func__, ret);
    return ret;
}

enum NORFLASH_API_RET_T norflash_api_write(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                const uint8_t *buffer,
                uint32_t len,
                bool async
                )
{
    MODULE_INFO *mod_info;
    MODULE_INFO *cur_mod_info;
    uint32_t lock;
    int32_t result;
    bool bresult;
    enum NORFLASH_API_RET_T ret;

    NORFLASH_API_TRACE("%s: mod_id = %d,start_addr = 0x%x,len = 0x%x.",
                __func__,mod_id,start_addr,len);

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE("%s: norflash_api uninit!",__func__);
        return NORFLASH_API_ERR_UNINIT;
    }

    if(mod_id > NORFLASH_API_MODULE_ID_COUNT)
    {
        NORFLASH_API_TRACE("%s : mod_id error! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_BAD_MOD_ID;
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_inited)
    {
        NORFLASH_API_TRACE("%s :module unregistered! mod_id = %d.",__func__, mod_id);
        return NORFLASH_API_ERR_UNINIT;
    }


    if(start_addr < mod_info->mod_base_addr
       || start_addr + len > mod_info->mod_base_addr + mod_info->mod_len)
    {
        NORFLASH_API_TRACE("%s : writting out of range! start_address = 0x%x,len = 0x%x.",
                    __func__,start_addr,len);
        return NORFLASH_API_BAD_ADDR;
    }

    if(len == 0)
    {
        NORFLASH_API_TRACE("%s : len error! len = %d.",
                   __func__,len);
        return NORFLASH_API_BAD_LEN;
    }
#if defined(FLASH_API_SIMPLE)
    async = false;
#endif
    if(async)
    {
        // add to opera_info chain header.
        lock = int_lock_global();
        result = _w_opera_add(mod_info,start_addr,len,(uint8_t*)buffer);
        if(result == 0)
        {
            ret = NORFLASH_API_OK;
        }
        else
        {
            ret = NORFLASH_API_BUFFER_FULL;
        }
        int_unlock_global(lock);
        NORFLASH_API_TRACE("%s: _w_opera_add done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    else
    {
        lock = int_lock_global();

        // flush the opera of currently being processed.
        if(norflash_api_info.cur_mod != NULL
            && mod_info != norflash_api_info.cur_mod)
        {
            cur_mod_info = norflash_api_info.cur_mod;
            while(cur_mod_info->state != NORFLASH_API_STATE_IDLE)
            {
                bresult = _opera_flush(cur_mod_info,true);
                if(!bresult)
                {
                    norflash_api_info.cur_mod = NULL;
                }
            }
        }

        // flush all of cur module opera.
        do{
            bresult = _opera_flush(mod_info,true);
        }while(bresult);
        pmu_flash_write_config();
        result = hal_norflash_write(mod_info->dev_id,start_addr, buffer, len);
        pmu_flash_read_config();
        if(result == HAL_NORFLASH_OK)
        {
            ret = NORFLASH_API_OK;
        }
        else if(result == HAL_NORFLASH_BAD_ADDR)
        {
            ret = NORFLASH_API_BAD_ADDR;
        }
        else if(result == HAL_NORFLASH_BAD_LEN)
        {
            ret = NORFLASH_API_BAD_LEN;
        }
        else
        {
            ret = NORFLASH_API_ERR;
        }
        int_unlock_global(lock);
        NORFLASH_API_TRACE("%s: hal_norflash_write done. start_addr = 0x%x,len = 0x%x,ret = %d.",
                __func__,start_addr,len,ret);
    }
    return ret;
}

// -1: error, 0:all pending flash op flushed, 1:still pending flash op to be flushed
int norflash_api_flush(void)
{
    enum NORFLASH_API_MODULE_ID_T mod_id = NORFLASH_API_MODULE_ID_COUNT;
    MODULE_INFO *mod_info;
    uint32_t lock;
    bool bret;

    if(!norflash_api_info.is_inited)
    {
        NORFLASH_API_TRACE("%s: norflash_api uninit!",__func__);
        return -1;
    }
#if defined(FLASH_API_SIMPLE)
    return 0;
#endif

    mod_info = _get_cur_mod();
    if(!mod_info)
    {
        return 0;
    }
    mod_id = _get_mod_id(mod_info);

    lock = int_lock_global();
    norflash_api_info.cur_mod_id = mod_id;
    norflash_api_info.cur_mod = mod_info;
    bret = _opera_flush(mod_info,false);
    if(!bret)
    {
        norflash_api_info.cur_mod = NULL;
    }
    int_unlock_global(lock);

    return 1;
}

bool norflash_api_is_changed(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                uint32_t start_addr,
                const uint8_t *buffer,
                uint32_t len)
{
    MODULE_INFO *mod_info;

    if(len == 0 || buffer == NULL)
    {
        ASSERT(0, "%s : parameter error! len = %d,buffer = 0x%x.",
                __func__,len,buffer);
    }
    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        ASSERT(0, "%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_inited)
    {
        ASSERT(0, "%s : module unregistered! mod_id = %d.",__func__, mod_id);
    }

    if(start_addr < mod_info->mod_base_addr
       || start_addr + len > mod_info->mod_base_addr + mod_info->mod_len)
    {
        ASSERT(0,"%s : reading out of range! start_address = 0x%x,len = 0x%x.",
                    __func__,start_addr,len);
    }

    if(_opera_cmp(mod_info,start_addr,(uint8_t*)buffer,len) == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool norflash_api_buffer_is_free(
                enum NORFLASH_API_MODULE_ID_T mod_id)
{
    MODULE_INFO *mod_info;
    uint32_t count;

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_inited)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }

    count = _get_ew_count(mod_info);
    if(count > 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

uint32_t norflash_api_get_used_buffer_count(
                enum NORFLASH_API_MODULE_ID_T mod_id,
                enum NORFLASH_API_OPRATION_TYPE type
                )
{
    MODULE_INFO *mod_info;
    uint32_t count = 0;

    if(mod_id >= NORFLASH_API_MODULE_ID_COUNT)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }

    mod_info = _get_module_info(mod_id);
    if(!mod_info->is_inited)
    {
        ASSERT(0,"%s : mod_id error! mod_id = %d.",__func__, mod_id);
    }
    if(type & NORFLASH_API_WRITTING)
    {
        count = _get_w_count(mod_info);
    }

    if(type & NORFLASH_API_ERASING)
    {
        count += _get_e_count(mod_info);
    }
    return count;
}

uint32_t norflash_api_get_free_buffer_count(
                enum NORFLASH_API_OPRATION_TYPE type
                )
{
    MODULE_INFO *mod_info;
    uint32_t i;
    uint32_t used_count = 0;
    uint32_t free_count = 0;

    if(type & NORFLASH_API_WRITTING)
    {
        for(i = NORFLASH_API_MODULE_ID_TRACE_DUMP; i < NORFLASH_API_MODULE_ID_COUNT; i ++)
        {
            mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
            if(mod_info->is_inited)
            {
                used_count += _get_w_count(mod_info);
            }
        }
        ASSERT(used_count <= NORFLASH_API_OPRA_LIST_LEN,"writting opra count error!");
        free_count += (NORFLASH_API_OPRA_LIST_LEN - used_count);
    }

    if(type & NORFLASH_API_ERASING)
    {
        for(i = NORFLASH_API_MODULE_ID_TRACE_DUMP; i < NORFLASH_API_MODULE_ID_COUNT; i ++)
        {
            mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
            if(mod_info->is_inited)
            {
                used_count += _get_e_count(mod_info);
            }
        }
        ASSERT(used_count <= NORFLASH_API_OPRA_LIST_LEN,"erase opra count error!");
        free_count += (NORFLASH_API_OPRA_LIST_LEN - used_count);
    }
    return free_count;
}


void norflash_flush_all_pending_op(void)
{
    int ret;
    do
    {
        ret = norflash_api_flush();
    } while (1 == ret);
}

