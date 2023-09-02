#ifndef SERVER_H
#define SERVER_H

#include <QtNetwork>
#include <QMap>
#include <QStringList>
#include <QObject>

class Server : public QObject {
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);

private slots:
    void handleNewConnection();
    void handleReadyRead();
    void handleDisconnected();
    void processReceivedData(QTcpSocket *socket, const QByteArray &data);
    void sendUserListToAll();
    void sendUserList(QTcpSocket* socket);
    void sendServerMessage(QTcpSocket* socket, const QString& message);
    void sendPrivateMessage(const QString& sender, const QString& receiver, const QString& message);
    void broadcastMessage(const QString& message);
    QString getUsernameFromSocket(QTcpSocket* socket);

private:
    QTcpServer* server;
    QList<QTcpSocket*> clients;
    QMap<QString, QTcpSocket*> clientUsernames;
    QMap<QString, QTcpSocket*> clientSockets;
};

#endif // SERVER_H
