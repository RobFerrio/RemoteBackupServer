//
// Created by rob on 03/12/20.
//

#include <fstream>
#include <filesystem>
#include "Connection.h"

std::mutex Connection::usersMutex;
std::unordered_map<std::string, std::tuple<std::string, std::string, bool>> Connection::users = {
        {"gold"     ,   {//"experience"   ,
                                "1bbb95740e28c2db088249a8bc27e4021921e00458f9e0c2e63bc6723374629dc6b641e8c2a8378c4999fc02a840c790e77f35d4875499dd968c2f913486fa27",
                                "a945ae55032d5e5840aa9ea59d85cc6f1ea1e56c76193c6de8cd006c9eb6ab964d7470710c9670de37a05e3fd31bdde070d181a8bf2a79408aebd522acee3ca3",
                                false}},
        {"sticky"   ,   {//"fingers"      ,
                                "6b4ca16370766a73d2b459caaa00f6bfec1e2fda3ca12700ee303788a15e5940a397528e911feb434e9773922dcebdbbfc3a54891e67966c6e0ad201cbd1a6f2",
                                "2053433695518814fe5d89338d2a29dba6cfebd52d515325985973227cd91ba0b07c519cb958af5605561674760b9177d773a571e21ce936fde7bccf653c6f85",
                                false}},
        {"moody"    ,   {//"blues"        ,
                                "60152acac4b02bce7cc824d5db743074ef6e798aeae5c0a197f02199b90764008bf19aac9a51585bc565ed39dd0d907ed19fcab5248c1d955efb2410d7fc44dd",
                                "cb0119df416abc93a400f88fb2ad4e397749ca75671acd1a143acdfb30827ceef7749c495514545525db8075cc10d68b7ea64af52acab4f000a453e8b1e5c07e",
                                false}},
        {"killer"   ,   {//"queen"        ,
                                "dbc8f167f84938aa6954c79f9d264ea672623b0f44da311527447b183b21e430a80aa44eb3ffb7364f9f9508d48e53599d8978c3c6430e2a8cc8001025b4b2cc",
                                "5b6d81004e48192e887fac53369fe3e0b81a931c78b276b44667350b8be334dedb2311dc255124a8fe908a244f59b5528cde6d40493b524a20081678a448308b",
                                false}},
        {"white"    ,   {//"album"        ,
                                "0c5bebf87d6a583f0e79a31dc9338b49d8c4d61f5c7ca539931ff375f82d451f335c4d0af0dcaef84cc7b179c23d7a44e8df11b45bec4572f7663df55f4702aa",
                                "4beb8f09736de327fa82fd90c4063cd2083df8bc40f68e38c814fcaf8b5bc23b009acbc5690de5ee732ae6808f51d22e6249aeaaa39434d2645903af8d020dfe",
                                false}},
        {"purple"   ,   {//"haze"         ,
                                "60bd6d5bc1ed8fb68d72fb54d51c600914a8c6615c4c920c83dc1746c2ba519a716abf2aad1b83ec220d8190335466f4c3b1c65ffaad1c2c17c99621db8ad073",
                                "96e1221f5dafeee9efd58b495534c442fe884b7e8321c1bcad5d23bdea1925d97da87fa7ed6a7637ee74465bad2d63293db2e70eb1acdf5e6432c4afd806d2c1",
                                false}},
        {"green"    ,   {//"day"          ,
                                "89ce878a69e325c0e0fbedd3af967ef89a87a113d7859706125d7911a041fe2201c48c312998e73261443d1b188679e3f6bb6b0bc1fc44567c83d4a53d7c86f1",
                                "668b65a97768263b00877c8dfc93ff7d9d88beeeb6cd5f90bd77123b05e40657fc1fb45cfe0ea8ac382e1949b84fec8756cbdee2954e6d4fbd4f0c6a273b219b",
                                false}}
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

    auto salt = std::get<0>(users[authData->first]);
    auto recomputedPassword = Message::compute_password(authData->second, salt, ITERATIONS, KEY_LENGTH);

    if(std::get<2>(users[authData->first]) || recomputedPassword != std::get<1>(users[authData->first]))
        return false;

    username = authData->first;
    std::get<2>(users[username]) = true;
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
                            self->handleFileRecv(std::make_shared<std::string>(std::move(path)));
                        }
                        break;
                    case DIR_SEND:
                        if (self->bufferMessage.checkHash() == 1) {
                            debug_cout("Creazione cartella");
                            debug_cout(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                            std::filesystem::create_directory(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                            self->listenMessages();
                        }
                        break;
                    case FILE_DEL:
                        if (self->bufferMessage.checkHash() == 1) {
                            debug_cout("Cancellazione file");
                            debug_cout(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                            std::filesystem::remove(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                            self->listenMessages();
                        }
                        break;
                    case DIR_DEL:
                        if (self->bufferMessage.checkHash() == 1) {
                            debug_cout("Cancellazione cartella");
                            debug_cout(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                            std::filesystem::remove_all(std::string(self->bufferMessage.getData().begin(), self->bufferMessage.getData().end()));
                            self->listenMessages();
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

void Connection::handleFileRecv(std::shared_ptr<std::string> pathPtr) {
    bufferMessage = Message();
    async_read([self = shared_from_this(), pathPtr](boost::system::error_code error, std::size_t bytes_transferred){
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
                        self->handleFileRecv(pathPtr);
                    }
                }
            } else {
                debug_cout(">>>errore ricezione file<<<");
                std::remove(pathPtr->data());
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
