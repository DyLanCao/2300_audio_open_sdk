#ifdef __INTERCONNECTION__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdint.h"
#include "hal_trace.h"

#include "umm_malloc.h"
#include "app_interconnection_tlv.h"
#include "app_interconnection.h"

#define  TYPESIZE (1)
#define  BUFSIZE (100)

#define  UINT16LEN (2)
#define  UINT32LEN (4)

static int is_local_env_little_endian(void)
{
    static int isLittle = -1;
    if(-1 == isLittle)
    {
        uint32_t test = 1;
        int8_t *p = (int8_t *)&test;
        if (1 == *p)
        {
            isLittle = 1;
        }
        else
        {
            isLittle = 0;
        }
    }

    return isLittle;
}

void app_interconnection_tlv_get_big_endian_u16_from_item(TLV_ITEM_T* item, uint16_t* dataPtr)
{
    uint8_t* buf = NULL;
    uint16_t ret = 0;
    uint8_t *r = (uint8_t *) &ret;
    uint8_t i = 0;

    if(NULL == item || NULL == item->value || NULL == dataPtr || UINT16LEN != item->length)
    {
        *dataPtr = 0;
        return;
    }

    buf = item->value;

    if (is_local_env_little_endian())
    {
        for (i = 0; i < UINT16LEN; i++)
        {
            *(r + i) = buf[UINT16LEN - i - 1];
        }
    }
    else
    {
        for (i = 0; i < UINT16LEN; i++)
        {
            *(r + i) = buf[i];
        }
    }

    *dataPtr = ret;
}

void app_interconnection_tlv_get_big_endian_u32_from_item(TLV_ITEM_T* item, uint32_t* dataPtr)
{
    uint8_t* buf = NULL;
    uint16_t ret = 0;
    uint8_t *r = (uint8_t *) &ret;
    uint8_t i = 0;

    if(NULL == item || NULL == item->value || NULL == dataPtr || UINT32LEN != item->length)
    {
        *dataPtr = 0;
        return;
    }

    buf = item->value;

    if (is_local_env_little_endian())
    {
        for (i = 0; i < UINT32LEN; i++)
        {
            *(r + i) = buf[UINT32LEN - i - 1];
        }
    }
    else
    {
        for (i = 0; i < UINT32LEN; i++)
        {
            *(r + i) = buf[i];
        }
    }

    *dataPtr = ret;
}

static void update_parent_length(TLV_ITEM_T *item)
{
    TLV_ITEM_T *p = item;
    uint32_t inCreaseLen = 0;
    uint32_t itemLen = 0;
    if(NULL == p)
    {
        return;
    }
    itemLen = item->length + TYPESIZE + app_interconnection_tlv_item_get_length_size(item->length);
    while (NULL != p->parentNode)
    {
        p->parentNode->length += itemLen + inCreaseLen;
        inCreaseLen = app_interconnection_tlv_item_get_length_size(itemLen + inCreaseLen);
        p = p->parentNode;
    }
}


uint32_t app_interconnection_tlv_item_get_length_size(uint32_t len)
{
    uint32_t size = 0;

    do
    {
        size++;
        len = len / CCMP_BIGLEN_MASK;
    }while(0 != len);

    return size;
}


TLV_ITEM_T *app_interconnection_creat_new_tlv_item(uint8_t type, uint32_t len, uint8_t *value)
{
    TLV_ITEM_T *node = (TLV_ITEM_T *)INTERCONNECTION_MALLOC(sizeof(TLV_ITEM_T));
    if(NULL == node)
    {
        //DBGPRINTF_HIGH("TlvItem Malloc Fail\n");
        //TRACE(DBG_ZONE_INTERFACE,("TlvItem Malloc Fail\r\n"));
        return NULL;
    }
    memset(node, 0, sizeof(TLV_ITEM_T));
    node->type = type;
    node->length = len;
    if(len > 0 && NULL != value)
    {
        node->value = (uint8_t *)INTERCONNECTION_MALLOC(len);
        if(NULL == node->value)
        {
            //DBGPRINTF_HIGH("TlvItem value Malloc Fail\n");
            //TRACE(DBG_ZONE_INTERFACE,("TlvItem value Malloc Fail\r\n"));
            INTERCONNECTION_FREE(node);
            return NULL;
        }
        memcpy(node->value, value, len);
        node->length = len;
    }
    return node;
}

void free_tlv_item(TLV_ITEM_T *item)
{
    if(NULL == item)
    {
        return;
    }
    if(NULL != item->value)
    {
        INTERCONNECTION_FREE(item->value);
        item->value = NULL;
    }
    INTERCONNECTION_FREE(item);
    item = NULL;
    return;
}

TLV_ITEM_T *app_interconnection_tlv_item_tree_malloc(uint8_t *buf, uint32_t len)
{
    uint8_t *p = buf;
    uint8_t temp = 0;
    uint8_t msg_type = 0;
    uint32_t msg_lenth = 0;
    uint8_t offset = 0;
    TLV_ITEM_T *item = NULL;
    if(NULL == p || 0 == len)
    {
        return NULL;
    }
    if(len > offset)
    {
        msg_type = (uint8_t)*p;
        offset++;
    }

    if(len > offset)
    {
        temp = (uint8_t)*(p + offset);
        msg_lenth = (~CCMP_BIGLEN_MASK) & temp;
        offset++;
        if((CCMP_BIGLEN_MASK & temp) && (offset < len))
        {
            temp = (uint8_t)*(p+offset);
            msg_lenth = msg_lenth * CCMP_BIGLEN_MASK + ((~CCMP_BIGLEN_MASK) & temp);
            offset++;
        }
    }

    if(msg_lenth > len - offset)
    {
        return NULL;
    }

    if (msg_type & CCMP_CHILD_MASK)
    {
        item = app_interconnection_creat_new_tlv_item((msg_type & ~CCMP_CHILD_MASK), msg_lenth, NULL);
        item->childNode = app_interconnection_tlv_item_tree_malloc((p + offset), msg_lenth);
        item->brotherNode = app_interconnection_tlv_item_tree_malloc((p + offset + msg_lenth), (len - msg_lenth - offset));
    }
    else
    {
        item = app_interconnection_creat_new_tlv_item((msg_type & ~CCMP_CHILD_MASK), msg_lenth, p + offset);
        item->childNode = NULL;
        item->brotherNode = app_interconnection_tlv_item_tree_malloc((p + offset + msg_lenth), (len - msg_lenth - offset));
    }

    if(item->childNode != NULL)
    {
        item->childNode->parentNode = item;
    }
    if(item->brotherNode != NULL)
    {
        item->brotherNode->parentNode = item->parentNode;
    }
    return item;
}

TLV_ITEM_T *app_interconnection_tlv_item_tree_malloc_long_msg(uint8_t *buf, uint32_t len)
{
    uint8_t *p = buf;
    uint8_t temp = 0;
    uint8_t msg_type = 0;
    uint32_t msg_lenth = 0;
    uint8_t offset = 0;
    TLV_ITEM_T *item = NULL;
    if(NULL == p || 0 == len)
    {
        return NULL;
    }
    if(len > offset)
    {
        msg_type = (uint8_t)*p;
        offset++;
    }

    if(len > offset)
    {
        temp = (uint8_t)*(p + offset);
        msg_lenth = (~CCMP_BIGLEN_MASK) & temp;
        offset++;
        if((CCMP_BIGLEN_MASK & temp) && (offset < len))
        {
            temp = (uint8_t)*(p+offset);
            msg_lenth = msg_lenth * CCMP_BIGLEN_MASK + ((~CCMP_BIGLEN_MASK) & temp);
            offset++;
        }
    }

    if(msg_lenth > len - offset)
    {
        msg_lenth = len - offset;
    }

    if (msg_type & CCMP_CHILD_MASK)
    {
        item = app_interconnection_creat_new_tlv_item(msg_type&~CCMP_CHILD_MASK, msg_lenth, NULL);
        item->childNode = app_interconnection_tlv_item_tree_malloc_long_msg((p + offset), msg_lenth);
        item->brotherNode = app_interconnection_tlv_item_tree_malloc_long_msg((p + offset + msg_lenth), (len - msg_lenth - offset));
    }
    else
    {
        item = app_interconnection_creat_new_tlv_item((msg_type & ~CCMP_CHILD_MASK), msg_lenth, (p + offset));
        item->childNode = NULL;
        item->brotherNode = app_interconnection_tlv_item_tree_malloc_long_msg((p + offset + msg_lenth), (len - msg_lenth - offset));
    }

    if(item->childNode != NULL)
    {
        item->childNode->parentNode = item;
    }
    if(item->brotherNode != NULL)
    {
        item->brotherNode->parentNode = item->parentNode;
    }
    return item;
}


void app_interconnection_tlv_item_tree_free(TLV_ITEM_T *tree)
{
    if(NULL == tree)
    {
        return;
    }

    app_interconnection_tlv_item_tree_free(tree->childNode);
    app_interconnection_tlv_item_tree_free(tree->brotherNode);
    free_tlv_item(tree);
}

void app_interconnection_tlv_item_print(uint32_t spaceNumber, TLV_ITEM_T *item)
{
    const char* format1 = "t:%d,l:%d";
    const char* format2 = ",v:";
    const char* format3 = "%x,";
    uint32_t len = 0;
    uint32_t i = 0;
    char* buf = NULL;
    char* value = NULL;
    char temp[BUFSIZE];

    if(NULL == item)
    {
        TRACE("app_interconnection_tlv_item_print item is NULL\r\n");
        return;
    }
    value = (char *)item->value;

    len = strlen(format1) + strlen(format2) + strlen(format3) * item->length + spaceNumber + 1;
    buf = (char *)INTERCONNECTION_MALLOC(len);
    if(NULL == buf)
    {
        TRACE("app_interconnection_tlv_item_print Malloc buf faild\r\n");
        return;
    }
    memset(buf, 0, len);
    memset(temp, 0 ,BUFSIZE);
    for (i = 0; i < spaceNumber; i++)
    {
        temp[i] = ' ';
    }
    strlcat(buf, temp, spaceNumber);

    memset(temp, 0 ,BUFSIZE);
    snprintf(temp, BUFSIZE, format1,item->type,item->length);
    strlcat(buf, temp, len);

    /*lint -e539*/
    for(i = 0; i < item->length && NULL != value; i++)
    {
        if(0 == i)
        {
             memset(temp, 0 ,BUFSIZE);
             snprintf(temp, BUFSIZE, format2);
             strlcat(buf, temp, len);
        }
        else
        {

        }
        memset(temp, 0 ,BUFSIZE);
        snprintf(temp, BUFSIZE, format3,value[i]);
        strlcat(buf, temp, len);
    }
    buf[len - 1] = '\0';
    TRACE("%s\r\n",buf);
    INTERCONNECTION_FREE(buf);
    buf = NULL;
    return;
}

void tlv_item_add_child(TLV_ITEM_T *parent, TLV_ITEM_T *child)
{
    TLV_ITEM_T *p = parent;
    if(NULL == p || NULL == child)
    {
        //TRACE("tlv_item_add_child NULL PARAM\r\n");
        return;
    }
    if(NULL != p->value)
    {
        //TRACE("Parent Value is Not NULL,is Leaf\r\n");
        return;
    }

    child->parentNode = p;

    if (NULL == p->childNode)
    {
        p->childNode = child;
        update_parent_length(child);
    }
    else
    {
        tlv_item_add_brother(p->childNode, child);
    }
}


void tlv_item_add_brother(TLV_ITEM_T *me, TLV_ITEM_T *brother)
{
    TLV_ITEM_T *m = me;
    if(NULL == m || NULL == brother)
    {
        return;
    }

    while(m->brotherNode != NULL)
    {
        m = m->brotherNode;
    }

    m->brotherNode = brother;
    brother->parentNode = m->parentNode;
    update_parent_length(brother);
}


TLV_ITEM_T *app_interconnection_tlv_get_item_with_type(TLV_ITEM_T *root, uint8_t type)
{
    TLV_ITEM_T *node = NULL;

    if (NULL == root)
    {
        return NULL;
    }

    if(type == root->type)
    {
        return root;
    }

    node = app_interconnection_tlv_get_item_with_type(root->childNode, type);

    if(NULL == node)
    {
        node = app_interconnection_tlv_get_item_with_type(root->brotherNode, type);
    }

    return node;
}

TLV_ITEM_T* app_interconnection_tlv_get_brother_node(TLV_ITEM_T *root)
{
    if(NULL == root)
    {
        return NULL;
    }
    return root->brotherNode;
}


TLV_ITEM_T* app_interconnection_tlv_get_child_node(TLV_ITEM_T *root)
{
    if(NULL == root)
    {
        return NULL;
    }
    return root->childNode;
}

uint8_t app_interconnection_tlv_get_item_type(TLV_ITEM_T *item)
{
    if(NULL == item)
    {
        return 0;
    }
    return item->type;
}


uint8_t* app_interconnection_tlv_get_item_value(TLV_ITEM_T *item,uint32_t *length)
{
    if(NULL == item)
    {
        *length = 0;
        return NULL;
    }
    if(NULL == item->value)
    {
        *length = 0;
    }
    else
    {
        *length = item->length;
    }
    return item->value;
}

static void app_interconnection_tlv_item_fill_buffer(TLV_ITEM_T *item, uint8_t *buf, uint32_t length)
{
    uint8_t *p = buf;
    uint32_t lenSize = 0;
    uint32_t itemLen = 0;
    uint32_t temp;
    uint32_t i = 0;
    if(NULL == item || NULL == p)
    {
        return;
    }
    if(0 == length)
    {
        return;
    }
    itemLen = item->length + app_interconnection_tlv_item_get_length_size(item->length) + TYPESIZE;
    if(itemLen > length)
    {
        return;
    }

    if(NULL == item->value && item->length > 0)
    {
        *p = (uint8_t)(item->type | CCMP_CHILD_MASK);
    }
    else
    {
        *p = (uint8_t)item->type;
    }
    p++;
    lenSize = app_interconnection_tlv_item_get_length_size(item->length);
    temp = item->length;
    for(i = 1; i <= lenSize; i++)
    {
        if(1 == i)
        {
            *(p + lenSize - i) = (uint8_t)(temp % CCMP_BIGLEN_MASK);
        }
        else
        {
            *(p + lenSize - i) = (uint8_t)((temp % CCMP_BIGLEN_MASK) | CCMP_BIGLEN_MASK);
        }
        temp = temp / CCMP_BIGLEN_MASK;
    }
    p += lenSize;

    if(NULL == item->value)
    {
        app_interconnection_tlv_item_fill_buffer(item->childNode,p,item->length);
    }
    else
    {
        memcpy(p, item->value, item->length);
    }
    p += item->length;
    app_interconnection_tlv_item_fill_buffer(item->brotherNode,p,length - itemLen);
    return;
}

uint32_t app_interconnection_tlv_get_item_tree_length(TLV_ITEM_T* root)
{
    uint32_t len = 0;
    TLV_ITEM_T *p = root;

    while(NULL != p)
    {
        len += p->length + TYPESIZE + app_interconnection_tlv_item_get_length_size(p->length);
        p = p->brotherNode;
    }

    return len;
}

uint8_t* app_interconnection_tlv_malloc_for_item(TLV_ITEM_T *item, uint32_t *length)
{
    uint32_t len = app_interconnection_tlv_get_item_tree_length(item);
    //TLV_ITEM_T *p = item;
    uint8_t *buf = NULL;

    if(0 == len)
    {
        //TRACE("item length is 0\r\n");

        *length = 0;
        return NULL;
    }

    buf = (uint8_t *)INTERCONNECTION_MALLOC(len);

    if(NULL == buf)
    {
        return NULL;
    }

    memset(buf, 0, len);
    *length = len;
    app_interconnection_tlv_item_fill_buffer(item, buf, len);
    return buf;
}

uint8_t* app_interconnection_tlv_malloc_for_item_tree_with_margin(TLV_ITEM_T *item, uint8_t head, uint8_t tail, uint32_t *length)
{
    uint32_t len = app_interconnection_tlv_get_item_tree_length(item);
    //TLV_ITEM_T *p = item;
    uint8_t *buf = NULL;

    if(0 == len)
    {
        *length = 0;
        return NULL;
    }

    len += (head + tail);

    buf = (uint8_t *)INTERCONNECTION_MALLOC(len);

    if(NULL == buf)
    {
        return NULL;
    }

    memset(buf, 0, len);
    *length = len;
    app_interconnection_tlv_item_fill_buffer(item, (buf + head), (len - head - tail));
    return buf;
}

#endif