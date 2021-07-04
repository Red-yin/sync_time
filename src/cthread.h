/*
 * CThread.h
 *
 *  Created on: 2020年3月5日
 *      Author: Administrator
 */

#ifndef JNI_VISPEECH_CTHREAD_H_
#define JNI_VISPEECH_CTHREAD_H_

#include <pthread.h>

class CThread
{
public:
	int start();
	virtual void run() = 0;

private:
	static void *start_thread(void *arg);

private:
	pthread_t pid;
};

#endif /* JNI_VISPEECH_CTHREAD_H_ */
