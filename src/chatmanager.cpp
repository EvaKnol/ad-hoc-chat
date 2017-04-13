#include "chatmanager.h"

#include <QMutableHashIterator> 


// Util: split string into vector
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

ChatManager::ChatManager(Router *r, ChatWindow *w, QObject *parent) : QObject(parent), router(r), chatwindow(w) {
    QSettings settings;

    name = settings.value("name", "Me").toString();
    ip_str = settings.value("ip").toString();
    my_ip = QHostAddress(ip_str).toIPv4Address();
    online = new QHash<quint32, quint32>();

    // Signals
    QObject::connect(r, &Router::messageReceived,
                     this, &ChatManager::handleMessage);
    QObject::connect(w, &ChatWindow::newMessage,
                     this, &ChatManager::sendMessage);
    QObject::connect(w, &ChatWindow::chatChanged,
                     this, &ChatManager::chatChanged);
    QObject::connect(w, &ChatWindow::recipientsChanged,
                     this, &ChatManager::recipientsChanged);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &ChatManager::notifyPresence);
    timer->start(5000);

    // Default Chat recipients for each chat tab:
    recipients = {
        {"192.168.5.1", "192.168.5.2", "192.168.5.3", "192.168.5.4"},
        {"192.168.5.1"},
        {"192.168.5.2"},
        {"192.168.5.3"},
        {"192.168.5.4"}
    };

    chatnames = {
        "Group Chat",
        "Hessel",
        "Eva",
        "Antonis",
        "Pepijn"
    };

    int rmv = -1;
    for(int c=0; c<chatnames.size(); c++){
        if (chatnames[c] == name){
            rmv = c;
            continue;
        }
        w->addChat(chatnames[c], false);
    }

    if (rmv >= 0){ // Remove the chat to myself
        recipients.erase(recipients.begin() + rmv);
        chatnames.erase(chatnames.begin() + rmv);
    }

    w->goToChat(0);
}

void ChatManager::handleMessage(pb::Packet p) {
    if (p.message_type() == pb::Packet::MESSAGE) {
        pb::Message msg = p.msg();

        if(msg.name() == name.toUtf8().constData())
            return;

        chatwindow->writeMessage(
            QString::fromStdString(msg.chatname()),
            QString::fromStdString(msg.name()),
            QString::fromStdString(msg.data()));
    } else if (p.message_type() == pb::Packet::PRESENCE) {
        online->insert(p.sender_ip(), 3);
    }
}

void ChatManager::sendMessage(QString chatname, QString message) {
    pb::Packet p;
    p.set_message_type(pb::Packet::MESSAGE);
    p.set_sender_ip(my_ip);

    pb::Message *msg = p.mutable_msg();
    msg->set_name(name.toStdString());
    msg->set_data(message.toStdString());

    for(int c=0; c<chatnames.size(); c++){
        if (chatname == chatnames[c]){
            for(auto r: recipients[c]){
                p.add_receiver_ip(QHostAddress(r).toIPv4Address());
            }

            msg->set_chatname(chatname.toUtf8().constData());
            router->sendMessage(p);
            break;
        }
    }
}

void ChatManager::notifyPresence() {
    pb::Packet p;
    p.set_message_type(pb::Packet::PRESENCE);
    p.set_sender_ip(my_ip);
    router->sendMessage(p);
    
    QMutableHashIterator<quint32, quint32> i(*online);
    while (i.hasNext()) {
        i.next();
        i.setValue(i.value()-1);
        if(i.value() == 0) i.remove();
    }
    emit isOnline(online->keys());
}

void ChatManager::chatChanged(int chatindex, StateChange change){
    /* Signal received that a chat changed */
    std::vector<QString> current_chat_names = chatwindow->getChatNames();
    std::vector<QString> default_vec = {"192.168.5.1", "192.168.5.2", "192.168.5.3", "192.168.5.4"};

    switch(change){
        case CREATED:
            qDebug() << "Chat Created: " << current_chat_names[chatindex] << "\n";
            recipients.push_back(default_vec);
            chatnames.push_back(current_chat_names[chatindex]);
            break;
        case RENAMED:
            qDebug() << "Chat `" << chatnames[chatindex] << "` ";
            chatnames[chatindex] = current_chat_names[chatindex];
            qDebug() << "renamed to " << current_chat_names[chatindex] << "\n";
            break;
        case DELETED:
            qDebug() << "Deleted Chat " << chatnames[chatindex] << "\n";
            chatnames.erase(chatnames.begin() + chatindex);
            recipients.erase(recipients.begin() + chatindex);
            break;
    }
}

void ChatManager::recipientsChanged(int chatindex, std::string new_rcpnts){
    /* Signal received that the recipients of a chat changed */
    std::vector<QString> new_recipients;

    std::vector<std::string> x = split(new_rcpnts, ',');
    qDebug() << "Changed recipients of " << chatnames[chatindex] << " to ";

    for(int r=0; r < x.size(); r++){ // Remove all spaces
        new_recipients.push_back(QString::fromStdString(x[r]).trimmed());
        qDebug() << new_recipients[r] << "\n";
    }

    recipients[chatindex] = new_recipients;
}