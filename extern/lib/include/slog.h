#ifndef _SLOG_H_
#define _SLOG_H_ 1
#include "com_log.h"

typedef enum log_type_t{
	MIN_TYPE=-1,
	_RAW,	
	_ERR,
	_WAR,
	_INF,	
	_DBG,
	_TRC,
	MAX_TYPE
}log_type_t;

typedef struct log_ctrl_t{
	char *name;
	char *color;
}log_ctrl_t;

void get_time_ms(char *ts,int size);
int log_init(const char *ctl_str);
void log_level_set(const char *level_name);

#define MAX_PNL 	64
extern char log_ctrl_set[MAX_TYPE+1];
extern char out_path_set[MAX_PNL];

void slog(
		log_type_t type,		//log 类型（级别）：
		char lock_en,			//是否启用互斥锁
		char *log_ctrl_set,		//log 开关控制
		const char *out_file,	//log 输出文件路径
		const char *ts,			//时间字符串
		const char *file,		//调用该函数的文件名
		const int line,			//调用该函数的行号
		const char *fmt,		//用于输出的格式化字符串
		...);
					
void raw_log(
		log_type_t type,
		char lock,
		char *log_ctrl_set,
		const char *out_path_set,
		const char *fmt,
		...);
				
#define tlog(type,lock,x...) ({\
	char ts[16]="";\
	get_time_ms(ts,sizeof(ts));\
	slog(type,lock,log_ctrl_set,out_path_set,ts,__FILE__,__LINE__,x);\
})

#define dlog(type,lock,x...) slog(type,lock,log_ctrl_set,out_path_set,"",__FILE__,__LINE__,x);

#define rlog(type,lock,x...) raw_log(type,lock,log_ctrl_set,out_path_set,x);

//这一组宏 都是带锁的，适用于多线程程序调试，单不能用于信号回调函数中
#define raw_l(x...) 	rlog(_RAW,1,x);//以原始方式打印，但是带锁，也可以进行开关操作
#define err_l(x...) 	dlog(_ERR,1,x);
#define war_l(x...) 	dlog(_WAR,1,x);
#define inf_l(x...) 	dlog(_INF,1,x);
#define dbg_l(x...) 	dlog(_DBG,1,x);
#define trc_l(x...) 	dlog(_TRC,1,x);

//这一组宏 都是不带锁的，适用于信号回调函数中，单不能用于多线程程序调试
#define raw(x...) 	rlog(_RAW,0,x);
#define err(x...) 	dlog(_ERR,0,x);
#define war(x...) 	dlog(_WAR,0,x);
#define inf(x...) 	dlog(_INF,0,x);
#define dbg(x...) 	dlog(_DBG,0,x);
#define trc(x...) 	dlog(_TRC,0,x);

#define err_t(x...) 	tlog(_ERR,1,x);
#define war_t(x...) 	tlog(_WAR,1,x);
#define inf_t(x...) 	tlog(_INF,1,x);
#define dbg_t(x...) 	tlog(_DBG,1,x);
#define trc_t(x...) 	tlog(_TRC,1,x);

#define show_errno(ret, msg) ({\
	int num=(ret);\
	if(!num)num=errno;\
	err("%s failure,errno:%d[%s]",msg,num,strerror(num));\
})
#endif
