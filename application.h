#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QSharedMemory>

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();
    bool lock();

private:
    QSharedMemory *_singular;
};

#endif // APPLICATION_H
