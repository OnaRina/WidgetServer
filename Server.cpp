#include "Server.h"
#include "ui_Server.h"

Server::Server(QObject *parent) : QObject(parent)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &Server::handleNewConnection);

    if (!server->listen(QHostAddress::Any, 12345))
    {
        qDebug() << "Server could not start!";
    }
    else
    {
        qDebug() << "Server started on port 12345";
    }
}

void Server::handleNewConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();

    // Read the username from the incoming connection
    if (socket->waitForReadyRead())
    {
        QByteArray usernameData = socket->readAll().trimmed();
        QString username = QString::fromUtf8(usernameData);

        // Check if the username is already in use
        if (clientUsernames.contains(username))
        {
            sendServerMessage(socket, "Username already in use. Please choose another.");
            socket->disconnectFromHost();
            socket->deleteLater();
        }
        else
        {
            // Add the client to the user list
            clients.append(socket);
            clientUsernames[username] = socket;

            connect(socket, &QTcpSocket::readyRead, this, &Server::handleReadyRead);
            connect(socket, &QTcpSocket::disconnected, this, &Server::handleDisconnected);

            // Notify all clients about the new user
            broadcastMessage(username + " has joined the chat.");
            sendUserListToAll();
        }
    }
}

void Server::handleReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    QByteArray data = socket->readAll();
    processReceivedData(socket, data);
}

void Server::handleDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket)
            return;

        QString username = getUsernameFromSocket(socket);

        clients.removeOne(socket);
        clientUsernames.remove(username);

        // Notify all clients about the disconnected user
        broadcastMessage(username + " has left the chat.");

        socket->deleteLater();
        sendUserListToAll();
}

void Server::processReceivedData(QTcpSocket *socket, const QByteArray &data)
{
    QString username = getUsernameFromSocket(socket);

    QString message = QString::fromUtf8(data).trimmed();
    if (message.startsWith("/"))
    {
        // Server command processing
        if (message == "/users")
        {
            sendUserList(socket);
        }
        else if (message.startsWith("/private "))
        {
            // Handle private messages
            QStringList parts = message.split(' ');
            if (parts.size() >= 3)
            {
                QString targetUser = parts[1];
                QString privateMessage = parts.mid(2).join(' ');
                sendPrivateMessage(username, targetUser, privateMessage);
            }
        }
        else
        {
            sendServerMessage(socket, "Unknown command: " + message);
        }
    }
    else
    {
        // Broadcast message
        broadcastMessage(username + ": " + message);
    }
}

void Server::sendUserListToAll()
{
    QStringList userList;
    for (const QString& username : clientUsernames.keys())
    {
        userList << username;
    }

    QByteArray userListData = userList.join('\n').toUtf8();
    for (QTcpSocket* client : clients)
    {
        client->write("/users\n");
        client->write(userListData);
        client->write("\n");
    }
}

void Server::sendUserList(QTcpSocket *socket)
{
    QStringList userList;
    for (const QString& username : clientUsernames.keys())
    {
        userList << username;
    }

    QByteArray userListData = userList.join('\n').toUtf8();
    socket->write("/users\n");
    socket->write(userListData);
    socket->write("\n");
}

void Server::sendServerMessage(QTcpSocket *socket, const QString &message)
{
    socket->write("/server\n");
    socket->write(message.toUtf8());
    socket->write("\n");
}

void Server::sendPrivateMessage(const QString &sender, const QString &receiver, const QString &message)
{
    QTcpSocket* targetSocket = clientUsernames.value(receiver);

    if (targetSocket)
    {
        targetSocket->write(("/private " + sender + " " + message + "\n").toUtf8());
    }
    else
    {
        sendServerMessage(clientUsernames[sender], "User not found: " + receiver);
    }
}

void Server::broadcastMessage(const QString &message)
{
    for (QTcpSocket* client : clients)
    {
        client->write(message.toUtf8());
        client->write("\n");
    }
}

QString Server::getUsernameFromSocket(QTcpSocket *socket)
{
    for (auto it = clientUsernames.begin(); it != clientUsernames.end(); ++it)
    {
        if (it.value() == socket)
        {
            return it.key();
        }
    }
    // Handle the case when the socket is not found
    qWarning() << "Socket not found in clientUsernames.";
    return QString("UnknownUser"); // Return a default value or an error indicator
}
