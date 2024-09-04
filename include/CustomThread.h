#ifndef CUSTOMTHREAD_H
#define CUSTOMTHREAD_H

#include <QThread>
#include <functional>
#include <QMutex>

class CustomThread : public QThread{
	Q_OBJECT

public:
	CustomThread(const std::function<void()>& task, QObject* parent = nullptr);

protected:
	void run() override;

private:
	std::function<void()> mTask; // ´æ´¢ÈÎÎñº¯Êı
	QMutex mMutex; // »¥³âËø
};

#endif