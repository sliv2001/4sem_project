#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QIODevice>
#include <QDataStream>
#include <QGraphicsObject>

QT_BEGIN_NAMESPACE
namespace Ui { class client; }
QT_END_NAMESPACE

class client : public QMainWindow
{
    Q_OBJECT

public:
    client(QWidget *parent = nullptr);
    ~client();

private slots:
    void on_actionQuit_triggered();

    void on_actionOpen_triggered();

    void on_actionRecognize_triggered();
    void on_sfdConnected();
    void on_sfdDisconnected();
    void on_sfdReadyRead();
    void on_sfdDisplayError(QAbstractSocket::SocketError socketError);

    void on_actionReconnect_triggered();

protected:
    virtual void resizeEvent(QResizeEvent *);

private:
    Ui::client *ui;
    QLabel* stat;
    QLabel* stat2;
    QTcpSocket *sfd;
    quint16 _blockSize;
    QString _name;
    int sendFile();
    void drawRect(float, float, float, float, QString, float);
};
#endif // CLIENT_H
