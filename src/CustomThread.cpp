#include "CustomThread.h"

CustomThread::CustomThread(const std::function<void()>& task, QObject* parent)
	: mTask(task)
	, QThread(parent) {

}

void CustomThread::run() {
	if (mTask) {
		QMutexLocker locker(&mMutex);
		mTask(); // 执行传入的任务
	}
}
