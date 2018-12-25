#ifndef UTIL_H
#define UTIL_H

int   countChar(char *str, char c);         //统计一串字符串中某个字符的个数
int   indexOfStrFirst(char *str, char c);   //查询字符首次出现的位置
char *getFileType(const char *filename);    //获取对应的HTML的响应头的文件类型
int   getFileSize(const char *filename);    //获取文件大小

#endif
