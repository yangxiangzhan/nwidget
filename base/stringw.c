#include <stddef.h>
#include <assert.h>
#include "stringw.h"

/**
  * @brief    获取一串字符串的显示宽度
  * @param    str : 字符串
  * @return   显示宽度
*/
int wstrnlen(const char *str,size_t len)
{
	int char_display_width ,char_width;
	int display_width = 0;
	const unsigned char *ptr = (const unsigned char *)str;
	if (!str) {
		return 0;
	}

	while(*ptr && len > 0) {
		char_width = utf8len(ptr[0]);
		char_display_width = char_width > 2 ? 2 : 1;
		display_width += char_display_width;
		for (int i = 0 ; i < char_width ; i++) {
			if (*++ptr == '\0')
				break ;
		}
		len -= char_width;
	}

	return display_width;
}


/**
  * @brief    获取一串字符串的显示宽度
  * @param    str : 字符串
  * @return   显示宽度
*/
int wstrlen(const char *str)
{
	int char_display_width ,char_width , key;
	int display_width = 0;
	const unsigned char *ptr = (const unsigned char *)str;
	if (!str) {
		return 0;
	}
	
	while(*ptr) {
		char_width = 1;
		char_display_width = 1;
		key = ptr[0] & 0xe0;
		if (key == 0xe0) {
			/* UTF8 占 3 个字节字体，此处假设都为中文，则占两个字节的显示长度 */
			char_width = 3;
			char_display_width = 2 ;
		} else 
		if (key == 0xc0){
			char_width = 2 ;
			char_display_width = 2 ;
		}

		display_width += char_display_width;
		for (int i = 0 ; i < char_width ; i++) {
			if (*++ptr == '\0')
				break ;
		}
	}

	return display_width;
}


/**
  * @brief    根据显示宽度复制一串字符串
  * @param    dest   : 目标字符串
  * @param    src    : 源字符串
  * @param    display_width : 一般为 dest 的内存大小
  * @return   成功 dest 字符串长度
*/
int wstrncpy(char *dest, const char *src,int display_width)
{
	unsigned char *ptr , *value;
	int char_width , char_display_width , key  ;
	
	assert(dest && display_width > 0);

	ptr = (unsigned char *)(src ? src : "NULL");
	value = (unsigned char *)dest;
	/* display_width -= 1; for '\0' */
	while(*ptr) {
		char_width = 1;
		char_display_width = 1;
		key = ptr[0] & 0xe0;
		if (key == 0xe0) {
			/* UTF8 占 3 个字节字体，此处假设都为中文，则占两个字节的显示长度 */
			char_width = 3;
			char_display_width = 2 ;
		} else if (key == 0xc0){
			char_width = 2 ;
			char_display_width = 2 ;
		}

		if (display_width < char_display_width) {
			break;
		}

		display_width -= char_display_width;
		for( ; *ptr && char_width-- ; *value++ = *ptr++) ;
	}

	*value = '\0' ;
	return (char *)value - dest;
}
