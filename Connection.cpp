//
// Created by rob on 03/12/20.
//

#include "Connection.h"

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

void Connection::handleConnection() {
    bufferMessage = Message(0);
    async_write(bufferMessage, [self = shared_from_this()](boost::system::error_code error, std::size_t bytes_transferred){
        if(!error){
            std::cout << "Write riuscita!" << std::endl;
            self->bufferMessage = Message();
            self->async_read([self](boost::system::error_code error, std::size_t bytes_transferred){
                if(!error) {
                    std::cout << "tipo: " << self->bufferMessage.getType() << " dimensione: " << self->bufferMessage.getSize() << std::endl;
                    std::cout << "hash: " << self->bufferMessage.checkHash() << std::endl;
                } else{
                    std::cout << error.message() << std::endl;
                }
            });
        }
    });
}