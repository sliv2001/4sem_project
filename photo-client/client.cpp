#include "client.h"
#include "ui_client.h"

QGraphicsScene* SIMG;
QString file = "";
int w_image, h_image;
qint64 volume=0;
bool same=1;

client::client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::client)
{
    ui->setupUi(this);

    SIMG = new QGraphicsScene(this);

    stat = new QLabel(this);
    stat2 = new QLabel(this);
    ui->statusbar->addWidget(stat);
    ui->statusbar->addWidget(stat2);
    stat->setText("Choose file by clicking \"File\\Open\" ");

    sfd = new QTcpSocket(this);
    auto res = connect(sfd, SIGNAL(readyRead()), this, SLOT(on_sfdReadyRead()));
    res = connect(sfd, SIGNAL(connected()), this, SLOT(on_sfdConnected()));
    res = connect(sfd, SIGNAL(disconnected()), this, SLOT(on_sfdDisconnected()));
    res = connect(sfd, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(on_sfdDisplayError(QAbstractSocket::SocketError)));
    sfd->connectToHost("127.0.0.1", 54322);
}

client::~client()
{
    delete SIMG;
    delete ui;
}


void client::on_actionQuit_triggered()
{
    QApplication::quit();
}

void client::on_actionOpen_triggered()
{
    QString str;
    str = QFileDialog::getOpenFileName(0, "Open Dialog", "", "*.jpg *.JPG *.png *.BMP *.bmp");
    QPixmap pixmap;
    if (str!=""){
        SIMG->clear();
        pixmap.load(str);
        SIMG->addPixmap(pixmap);
        SIMG->setSceneRect(0, 0, pixmap.width(), pixmap.height());
        ui->scene->setScene(SIMG);
        w_image = pixmap.width();
        h_image = pixmap.height();
        ui->scene->fitInView(0, 0, w_image, h_image, Qt::KeepAspectRatio);
        file = str;
        stat->setText(file);
        same=0;
    }
}

int client::sendFile(){
    QFile f = QFile(file);
    if (f.open(QIODevice::ReadOnly)){
        uint64_t size = f.size();
        sfd->write((char*)&size, 8);
        auto data = f.readAll();
        sfd->write(data);
    }
    f.close();
}

void client::on_actionRecognize_triggered()
{
    same=1;
    if (sfd->state()==QAbstractSocket::ConnectedState&&file!=""){
        sendFile();
    }
}

void client::resizeEvent(QResizeEvent * sz){
    ui->scene->fitInView(0, 0, w_image, h_image, Qt::KeepAspectRatio);
}

void client::drawRect(float ymin, float xmin, float ymax, float xmax, QString str, float p){
    SIMG->addRect(xmin*SIMG->width(), ymin*SIMG->height(), (xmax-xmin)*SIMG->width(), (ymax-ymin)*SIMG->height());
    QString result;
    QTextStream(&result) << str << " " << p;
    QGraphicsSimpleTextItem* text = SIMG->addSimpleText(result);
    text->setPos(xmin*SIMG->width(), ymin*SIMG->height());
    text->setScale(SIMG->width()/550);
    QBrush b=text->brush();
    b.setColor(Qt::red);
    text->setBrush(b);
}

/*---------- Socket stuff --------------*/

void client::on_sfdConnected()
{
    stat2->setText("Ready");
}

void client::on_sfdReadyRead()
{
    do{
    if (volume==0&&sfd->bytesAvailable()>=8){
        sfd->read((char*)&volume, 8);
        if (volume==0)
            return;
    }
    if (volume>sfd->bytesAvailable())
        return;
    float xmin, xmax, ymin, ymax, p;
    char str[128];
    sfd->read((char*)&ymin, 4);
    sfd->read((char*)&xmin, 4);
    sfd->read((char*)&ymax, 4);
    sfd->read((char*)&xmax, 4);

    int i=0;
    char a=1;
    while (a!=0){
        sfd->read(&a, 1);
        str[i]=a;
        i++;
    }

    sfd->read((char*)&p, 4);
    QString qstr = QString(str);
    if (same)
        drawRect(ymin, xmin, ymax, xmax, qstr, p);
    volume=0;

    }while(sfd->bytesAvailable()>=8);
}

void client::on_sfdDisconnected(){

}

void client::on_sfdDisplayError(QAbstractSocket::SocketError error){
    stat2->setText(sfd->errorString()+" ");
}

void client::on_actionReconnect_triggered()
{
    if(sfd->state()==QAbstractSocket::ConnectedState)
        sfd->disconnectFromHost();
    sfd->connectToHost("127.0.0.1", 54322);
}

