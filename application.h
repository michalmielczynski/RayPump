#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QSharedMemory>
#include <QLockFile>
#include <QProcessEnvironment>
#include <QDir>
#include <QMessageBox>
#include <QSettings>

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();
    bool lock();

private:
    QLockFile *_singular;
};

#endif // APPLICATION_H
