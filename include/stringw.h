#ifndef _STR_WIDE_
#define _STR_WIDE_

#define utf8_display_width()

static inline int utf8len(unsigned char utf8head)
{
	int len = 0;
	if (utf8head < 0x80) {
		return 1;
	}
	if ((utf8head & 0xf0) == 0xf0) {
		len += 4;
		utf8head <<= 4;
	}
	if ((utf8head & 0xc0) == 0xc0) {
		len += 2;
		utf8head <<= 2;
	}
	if ((utf8head & 0x80) == 0x80) {
		len += 1;
	}
	return len;
}


/**
  * @brief    获取一串字符串的显示宽度
  * @param    str : 字符串
  * @return   显示宽度
*/
int wstrlen(const char *str);


/**
  * @brief    获取一串字符串的显示宽度
  * @param    str : 字符串
  * @return   显示宽度
*/
int wstrnlen(const char *str,size_t len);

/**
  * @brief    根据显示宽度复制一串字符串
  * @param    dest   : 目标字符串
  * @param    src    : 源字符串
  * @param    display_width : 显示长度
  * @return   成功 dest 字符串长度
*/
int wstrncpy(char *dest, const char *src,int display_width);

#endif /* _STR_WIDE_ */