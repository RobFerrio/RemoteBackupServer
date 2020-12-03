#include <iostream>
#include "BackupServer.h"

int main() {
    try {
        boost::asio::io_context ioContext;
        BackupServer server(ioContext, 5000);

        std::vector<std::thread> threads;
        //Creo tanti thread quanti ne supporta la CPU concorrentemente, e in ognuno chiamo ioContext.run().
        //In questo modo ha un comportamento uguale a un thread pool: i completion handler sono invocati nel primo thread disponibile che ha chiamato la run()
        for(int n = 0; n < std::thread::hardware_concurrency(); ++n){
            threads.emplace_back([&]{ ioContext.run(); });
        }
        //se in futuro ci fosse un modo per stoppare il server si passerebbe da qui
        for(auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    catch (std::exception& e) {
        std::cerr<< e.what() << std::endl;
    }
    return 0;
}
