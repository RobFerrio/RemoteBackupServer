//
// Created by rob on 03/12/20.
//

#include <fstream>
#include <filesystem>
#include "Connection.h"

std::mutex Connection::usersMutex;
std::unordered_map<std::string, std::pair<std::string, bool>> Connection::users = {
        {"gold"     ,   {"experience"   ,false}},
        {"sticky"   ,   {"fingers"      ,false}},
        {"moody"    ,   {"blues"        ,false}},
        {"killer"   ,   {"queen"        ,false}},
        {"white"    ,   {"album"        ,false}},
        {"purple"   ,   {"haze"         ,false}},
        {"green"    ,   {"day"          ,false}}
};

template<typename T, typename Handler>
void Connection::async_write(const T& t, Handler handler) {
    //Serializza i dati
    std::ostringstream archiveStream;
    boost::archive::text_oarchive archive(archiveStream);
    archive << t;
    outboundData = archiveStream.str();

    //Formatta l'header
    std::ostringstream headerStream;
    headerStream << std::setw(HEADER_LENGTH) << std::hex << outboundData.size();
    if(!headerStream || headerStream.str().size() != HEADER_LENGTH){
        //Errore creazione header, ritorna errore all'handler
        boost::system::error_code error(boost::asio::error::invalid_argument);
        boost::asio::post(ioContext, std::bind(handler, error, 0));     //Si affida il completion handler all'io_context
        return;
    }
    outboundHeader = headerStream.str();

    //Componi il buffer header + data e spediscilo (uso array e non vector perchè la dimensione è fissa => meno overhead)
    std::array<boost::asio::const_buffer, 2> buffer{boost::asio::buffer(outboundHeader), boost::asio::buffer(outboundData)};
    boost::asio::async_write(socket, buffer, handler);
}

template<typename Handler>
void Connection::async_read(Handler handler) {
    boost::asio::async_read(socket, boost::asio::buffer(inboundHeader),
            [self = shared_from_this(), handler](boost::system::error_code error, std::size_t bytes_transferred){
        //  >>Lettura header<<
        if(error)
            boost::asio::post(self->ioContext, std::bind(handler, error, 0));
        else{
            //Elabora lunghezza dati
            std::istringstream iss(std::string(self->inboundHeader, HEADER_LENGTH));
            size_t inboundDataSize = 0;
            if(!(iss >> std::hex >> inboundDataSize)){  //Formato header errato
                error = boost::asio::error::invalid_argument;
                boost::asio::post(self->ioContext, std::bind(handler, error, 0));
                return;
            }
            //Inizia lettura dati
            self->inboundData.resize(inboundDataSize);
    boost::asio::async_read(self->socket, boost::asio::buffer(self->inboundData),
            [self, handler](boost::system::error_code error, std::size_t bytes_transferred){
        //  >>Lettura dati<<
        if(!error){
            try {
                //Deserializza
                std::string archiveData(self->inboundData.begin(), self->inboundData.end());    //vector -> string
                std::istringstream archiveStream(archiveData);                                  //string -> stream
                boost::archive::text_iarchive archive(archiveStream);                       //stream -> archive
                archive >> self->bufferMessage;
            } catch (std::exception& e) {
                //Problema nella deserializzazione
                error = boost::asio::error::invalid_argument;
                boost::asio::post(self->ioContext, std::bind(handler, error, 0));
                return;
            }
        }
        //Comunica risultato all'handler
        boost::asio::post(self->ioContext, std::bind(handler, error, bytes_transferred));
    });
        }
    });
}

void Connection::doHandshake() {
    socket.async_handshake(boost::asio::ssl::stream_base::server, [self = shared_from_this()](const boost::system::error_code& error){
        if(!error)
            self->handleConnection();
    });
}

void Connection::handleConnection() {
    //Invia richiesta autenticazione
    bufferMessage = Message(AUTH_REQ);
    async_write(bufferMessage, [self = shared_from_this()](boost::system::error_code error, std::size_t bytes_transferred){
        if(!error){
            self->bufferMessage = Message();
            //Leggi username/psw
            self->async_read([self](boost::system::error_code error, std::size_t bytes_transferred){
                if(!error) {
                    try{
                        auto authData = self->bufferMessage.extractAuthData();
                        if(self->login(authData)){
                            self->folder = Folder("../" + self->username);
                            self->bufferMessage = Message(self->folder.getPaths());
                            //Conferma autenticazione + invia immagine folder
                            self->async_write(self->bufferMessage, [self](boost::system::error_code error, std::size_t bytes_transferred) {
                                if (!error) {
                                    self->bufferMessage = Message();
                                    //Leggi immagine folder del client
                                    self->async_read([self](boost::system::error_code error, std::size_t bytes_transferred){
                                        self->handleFileList();
                                    });
                                }
                            });
                        }
                    }catch(std::exception& e){
                        safe_cout(e.what());
                    }
                }
            });
        }
    });
}

bool Connection::login(std::optional<std::pair<std::string, std::string>>& authData) {
    if(!authData.has_value())
        return false;

    std::lock_guard<std::mutex> lg(usersMutex);
    if(!users.contains(authData->first))
        return false;
    if(users[authData->first].second || users[authData->first].first != authData->second)
        return false;

    username = authData->first;
    users[username].second = true;
    return true;
}

void Connection::listenMessages() {
    debug_cout("listenMessages");
    bufferMessage = Message();
    async_read([self = shared_from_this()](boost::system::error_code error, std::size_t bytes_transferred){
        if(!error) {
            try {
                switch (self->bufferMessage.getType()) {
                    case FILE_START:
                        if (self->bufferMessage.checkHash() == 1) {
                            std::string path(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end());
                            debug_cout(path);
                            std::ofstream ofs(path, std::ios::binary | std::ios_base::trunc);     //Crea il file o azzeralo prima di cominciare
                            self->handleFileRecv(std::move(path));
                        }
                        break;
                    case DIR_SEND:
                        if (self->bufferMessage.checkHash() == 1) {
                            try {
                                debug_cout("Creazione cartella");
                                debug_cout(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                                std::filesystem::create_directory(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                                self->listenMessages();
                            } catch (std::exception &e) {
                                safe_cout(e.what());
                            }
                        }
                        break;
                    case FILE_DEL:
                        if (self->bufferMessage.checkHash() == 1) {
                            try{
                                debug_cout("Cancellazione file");
                                debug_cout(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                                std::filesystem::remove(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                                self->listenMessages();
                            } catch (std::exception &e) {
                                safe_cout(e.what());
                            }
                        }
                        break;
                    case DIR_DEL:
                        if (self->bufferMessage.checkHash() == 1) {
                            try{
                                debug_cout("Cancellazione cartella");
                                debug_cout(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                                std::filesystem::remove_all(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                                self->listenMessages();
                            } catch (std::exception &e) {
                                safe_cout(e.what());
                            }
                        }
                        break;
                }
            }catch(std::exception& e){
                safe_cout(e.what());
            }
        }
    });
}

void Connection::handleFileList() {
    if(bufferMessage.checkHash() == 0)
        return;

    auto clientFolder = bufferMessage.extractFileList();
    if(!clientFolder.has_value())
        return;

    auto folderDiffs = folder.compare(clientFolder.value());
    handleDiffs(std::make_shared<std::map<std::string, int>>(std::move(folderDiffs)));
}

void Connection::handleDiffs(std::shared_ptr<std::map<std::string, int>> diffs) {
    if(diffs->empty())
        listenMessages();
    else{
        switch (diffs->begin()->second) {
            case CLIENT_MISSING_DIR:
                debug_cout("in client missing dir");
                debug_cout(diffs->begin()->first);
                bufferMessage = Message(DIR_SEND, diffs->begin()->first);
                diffs->erase(diffs->begin());
                async_write(bufferMessage,[self = shared_from_this(), diffs](boost::system::error_code error, std::size_t bytes_transferred){
                    if(!error)
                        self->handleDiffs(diffs);
                });
                break;
            case CLIENT_MISSING_FILE:
                debug_cout("in client missing file");
                debug_cout(diffs->begin()->first);
                std::ifstream ifs(diffs->begin()->first, std::ios::binary);
                if (ifs.is_open()){
                    bufferMessage = Message(FILE_START, diffs->begin()->first);
                    diffs->erase(diffs->begin());
                    async_write(bufferMessage,[self = shared_from_this(), diffs, openIfs = std::make_shared<std::ifstream>(std::move(ifs))](boost::system::error_code error, std::size_t bytes_transferred){
                        if(!error)
                            self->sendFile(openIfs, diffs);
                    });
                }
                break;
        }
    }
}

void Connection::handleFileRecv(std::string path) {
    bufferMessage = Message();
    async_read([self = shared_from_this(), pathPtr = std::make_shared<std::string>(std::move(path))](boost::system::error_code error, std::size_t bytes_transferred){
        try {
            if (!error) {
                if (self->bufferMessage.getType() == FILE_END) {
                    debug_cout(">>>fine ricezione file<<<");
                    self->listenMessages();
                } else if (self->bufferMessage.getType() != FILE_DATA || self->bufferMessage.checkHash() != 1) {
                    debug_cout(">>>errore ricezione file<<<");
                    std::remove(pathPtr->data());
                } else {
                    debug_cout(">ricezione file chunk");
                    std::ofstream ofs(pathPtr->data(), std::ios::binary | std::ios_base::app);
                    if (ofs.is_open()) {
                        ofs.write(self->bufferMessage.getData().data(), self->bufferMessage.getData().size());
                        self->handleFileRecv(*pathPtr);
                    }
                }
            }
        } catch (std::exception& e) {
            safe_cout(e.what());
        }
    });
}

void Connection::sendFile(std::shared_ptr<std::ifstream> ifs, std::shared_ptr<std::map<std::string, int>> diffs) {
    try {
        if (!ifs->eof()) {
            debug_cout(">Send file chunk");
            std::vector<char> buffer(CHUNK_SIZE);
            ifs->read(buffer.data(), buffer.size());
            size_t size = ifs->gcount();
            if (size < CHUNK_SIZE)
                buffer.resize(size);
            bufferMessage = Message(FILE_DATA, buffer);
            async_write(bufferMessage, [self = shared_from_this(), diffs, ifs](boost::system::error_code error, std::size_t bytes_transferred) {
                if (!error)
                    self->sendFile(ifs, diffs);
            });
        } else {
            debug_cout(">>>Fine send file<<<");
            bufferMessage = Message(FILE_END);
            async_write(bufferMessage, [self = shared_from_this(), diffs](boost::system::error_code error, std::size_t bytes_transferred) {
                if (!error)
                    self->handleDiffs(diffs);
            });
        }
    } catch (std::exception& e) {
        safe_cout(e.what());
    }
}
